/********************************************************************************
 *  TapeReader.cpp  --  Investor/RT RTX extension  lsTapeReader  1.2.0  (2026-10-14)
 *  1.2.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, APPROVAL-LIST-2026-10-14.md section 3): line 1 no
 *        longer shows the BULLISH / BEARISH n of 4 bias (ClaimGates.h LRA_SHOW_TAPE_BIAS) or any turn / swing % ("Turn odds learning",
 *        LRA_SHOW_TURN_ODDS) - both still computed and logged; "Next: 6,030 PDH" stays. The hover: Pull lines end at the numbers (no
 *        "support / resistance leaving"), "Added at PDL: 42 bids, 150 lots (4.4x) fast, 90% re-added after hits", "reloading? (I 2.1x)",
 *        "iceberg?", "Last push to 6,005 on thin selling (Ex?)" (no "exhausted"), the OPTIONS lines keep every "?" (ODEX too) and end
 *        "(model, 7m old)". Logged LEVELS: "Bids 2.6x normal at PDL ?", "+GEX PDL 6,020 (model, 7m old) ?", "Tgt 6,030 PDH" (no %).
 *  1.1.4 (2026-10-14, Rassul 15:07 CT approval of the peer-review hide list): the Delta Profile A (absorption) is no longer a FLOW
 *        vote or a BUYERS / SELLERS message (ClaimGates.h, LRA_SHOW_ABSORPTION_REASON default 0 = hidden until the external audit;
 *        restore = /D LRA_SHOW_ABSORPTION_REASON=1 and rebuild). The A circle on the chart is lsDeltaProfile's and stays.
 *  1.1.3 (2026-10-14, CR4800; Rassul 07:12 "you already have messaging at the top", 07:13 decision "variant B, the one banner at the
 *        top", 07:17 solid dark background, 07:18 "test them thoroughly ... dont shake ... dont have garbage values"): NO big box. A
 *        compact 1-2 line BANNER at the top left, below the title row, 11 pt (10% under 1.1.2): line 1 = the version and the candle's
 *        time, the bias, the turn / swing odds and the next level; line 2 = the Liquidity Profile's warning (handed over through
 *        BannerBoard.h - the Liquidity Profile 0.15.3 stops drawing its own while this banner is on the chart). Hover over the banner =
 *        the BUYERS / SELLERS / OPTIONS detail (10 pt) right under it; the candle it reads stays while the mouse is on it. Settings:
 *        Position (default Top left), Offset X / Y (px, -600..600), Banner background (solid fill + 1 px border, default on) - kept per
 *        market in TapeReader.place-<MKT>.txt "pos,dx,dy,bg" (the dialog is filled from it when it opens; a control he did not touch
 *        keeps the file's value - the 07:07 "Top left" in the dialog was IRT's stale list value, the chart drew the file / default).
 *        Width stable (grows at once, shrinks after 60 s), hysteresis against the HOD / LOD table, never past the last bar.
 *        (CR5000 test pass) the hysteresis holds only the spot's anchor point, so a 1 -> 2 line change / a wider text shows on the same
 *        draw (no 1-draw lag); the hover has a 6 px dead band (opens inside only, closes 6 px outside the banner / its detail); the
 *        banner suite (jitter 1,000 draws, hover, board heartbeat, dialog -> file -> restart round trip, fuzz, two charts) passes.
 *        Settings that reset: setParameterVersion 2 -> IRT resets the saved Position row once; the chart never reads it (the place
 *        file TapeReader.place-<MKT>.txt is the only source, an old "7" file still means Bottom centre).
 *  1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md): default Position BOTTOM CENTRE;
 *        sections header / bias line / BUYERS (2 lines) / SELLERS (2 lines) / OPTIONS (2 lines, blue) - the LEVELS section is removed
 *        (levels are named inside BUYERS / SELLERS; the LEVELS text is still logged for the nightly). BUYERS / SELLERS use the panes'
 *        grammar (lots, ratio, pace): "Defending PDL: 42 bids, 150 lots (4.4x) added fast, 90% rebuilt", "Swept 3 lvls into PDL with a
 *        240-lot (3.4x), no follow-through", "Pulled 38 offers, 95 lots (3.6x) fast - resistance leaving". OPTIONS reads
 *        LRA-Options-<MKT>.csv (written by the LRA bridge after every MenthorQ snapshot) and the gates of LRA-OrderFlow-<MKT>.csv:
 *        each grid is used from the candle it ARRIVED on (never earlier) and only while <= 20 min old. Header shows v1.1.
 *        Keeps every 1.0.1 fix (GAMMA fallback / freshness, BOOKASOF, LRA-Turns v2, letter I, the user32 / kernel32 pragmas).
 *
 *  Rassul 2026-10-13: "add a reader box. call it tape reader ... give your read on what buyers and sellers are doing by reading the
 *  indicators as well as other things like levels, gamma, liquidity" / "split it up by buyers and sellers" / "the turn up swing and
 *  next stat should be centered" (approved mockup tape-reader-buyers-sellers-mockup.png, with line 1's stats centred);
 *  11:55 "BUYERS, SELLERS and LEVELS may each use up to 2 lines".
 *
 *  ONE box on the price pane (Settings > Position, 9 spots, default Bottom left), only in the space up to the last bar and clear of
 *  the profiles. Nothing else is drawn on the chart.
 *     Tape Reader v1.0  ·  CL  ·  10:30 AM                                   (header, centred: the candle's time, 12-hour Central)
 *     BULLISH 3 of 4        Turn up at LonLO 62%?   Swing 20+ ticks 41%?   Next: 93.20 PWH      (bias left, the stats centred)
 *     BUYERS   what buyers did (green / red = the implication of their action, light grey = quiet)          (1-2 lines)
 *     SELLERS  what sellers did                                                                              (1-2 lines)
 *     LEVELS   liquidity, gamma, HOD / LOD, against, Inval, Tgt                                              (1-2 lines)
 *  Every other indicator keeps its own local read; this box reads their files (TapeReaderLogic.h lists them) and writes one read.
 *  Four voters (FLOW, LEVEL, GAMMA, TREND), each bullish / bearish / neutral at the candle; per-market weights from the nightly
 *  LRA-TapeReader-<MKT>.csv (equal by default); the count shown is the simple agree count. Turn / swing odds from the nightly
 *  LRA-Turns-<MKT>.csv, file VERSION 2 ("Turn odds learning" until the bucket has n >= 20 and is past learning; '?' until proven).
 *  TIMING: recomputed at each candle close (finalised when the panes' rows for that candle have landed, at most 5 s later) and
 *  intrabar only when a new A / AE (Delta Profile) or S / I (order-flow panes) appears at a level - no other intrabar change.
 *  HOVER (TRACK_MOUSE): the read logged for the candle under the mouse; otherwise the live read. Never looks ahead.
 *  LOG: every finalised candle's read once to lsFlexLevels\TapeReader\<MKT>-<CONTRACT>-<spb>-<YYYYMMDD>.csv (the nightly scores it);
 *  status lsFlexLevels\TapeReader.status-<MKT>.txt (version, inputs found / stale, last read), atomic tmp -> bak -> rename.
 *  1.0.1 (2026-10-13, DEX audit, bug fixes only, no look change): GAMMA reads MenthorQ net GEX $ when the Dealer file's gamma column
 *  is 0 on every row (the SPX / QQQ index book in ES / NQ cash hours), else abstains ("?"); the Dealer file is judged by its DATA
 *  time - BOOKASOF (the grid's own time) when present, else ASOF; the SRC book time when older - at most 15 min old in RTH (18 h
 *  outside), else GAMMA abstains ("stale 18h" in the log / status). LINK FIX: #pragma comment(lib) user32 (GetCursorPos /
 *  WindowFromPoint) + kernel32 - his gex-build links only the SDK lib, BidAskStats 1.3.0 failed LNK2019 on the same two calls.
 *  LRA-Turns file VERSION 2 (COLS / ROW by name, UNIT xER, long/short, level names, p_swing_x, X x the live expected range).
 *  Refill / iceberg letter is I (was R; R is now stop run): old logs' R is read as I; the box says "reloading (I 5.2x)".
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "DealerLogic.h"
#include "VwapTrendLogic.h"
#include "TapeReaderLogic.h"
#include "TapeReaderSettings.h"     // (1.3.1) the ONE settings table (SettingsSpec.h)
#include "SettingsHost.h"           // (1.3.1) the window from the table
#include "HostSlot.h"
#include "HoverBoard.h"
#include "ZoneStory.h"                  // (1.3.0) the Market State (MarketState.h) + the zone story
#include "OddsTable.h"                  // (1.3.0) the probability engine's answers (LRA-Odds-<MKT>.csv)
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#define TR_VERSION "1.3.4"          // keep equal to tr::VERSION and the setVersion literal below
// (1.3.3, Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other places") the banner's BUYERS / SELLERS / OPTIONS detail popup on mouse-over is OFF (TR_BANNER_HOVER; code kept). TRACK_MOUSE stays
// only for the chart-wide candle follow (HoverBoard: the banner and the order-flow panes read the candle under the mouse) - no popup.
static const bool TR_BANNER_HOVER = false;

static const long long GRACE_MS = 5000;           // a closed candle is finalised when its pane rows landed, else 5 s after it closed
static const long long CHECK_FAST_MS = 1000;      // Delta / order-flow records (the intrabar triggers)
static const long long CHECK_SLOW_MS = 2000;      // everything else
static const long long ACTIVE_S = 600;            // an order-flow pane whose record changed in the last 10 min is expected to log every candle
static const size_t KEEP_BARS = 2000;
static const size_t MAX_HOVER_WNDS = 8;

#if defined(TR_TEST_CLOCK)
extern long long tr_test_now_ms, tr_test_unix;
extern void* tr_test_cursor_wnd;
extern int tr_test_fail_rename;
static long long steadyMs() { return tr_test_now_ms; }
static long long wallNow() { return tr_test_unix; }
static void* cursorWindow() { return tr_test_cursor_wnd; }
static int renameFile(const char* a, const char* b) { if (tr_test_fail_rename > 0) { --tr_test_fail_rename; return -1; } return std::rename(a, b); }
#else
static long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static long long wallNow() { return (long long)std::time(nullptr); }
#if defined(_WIN32)
// (1.0.1 link fix, 2026-10-13) his gex-build links no default Win32 libs beyond the CRT: BidAskStats 1.3.0 failed LNK2019 on exactly
// these two. Every Win32 API this file calls, by import lib: user32 = GetCursorPos / WindowFromPoint; kernel32 = CreateDirectoryA (makeDir) /
// GetFileAttributesExA (TapeReaderLogic fileSig). No gdi32: all drawing goes through the IRT SDK.
#if defined(_MSC_VER)                     // cl (his gex-build); MinGW / clang-gnu ignore #pragma comment
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#endif
static void* cursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
#else
static void* cursorWindow() { return NULL; }
#endif
static int renameFile(const char* a, const char* b) { return std::rename(a, b); }
#endif

static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
#if defined(_WIN32)
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\";
#else
    return std::string(up) + "/";                 // mock tests on Linux
#endif
}
static const char* SEP() {
#if defined(_WIN32)
    return "\\";
#else
    return "/";
#endif
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
// checklist #5: tmp, old -> bak, tmp -> path; the bak is restored when the final rename fails
static bool writeAtomic(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f << text; f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    std::remove(bak.c_str());
    const bool had = tr::fileSig(path) >= 0;
    if (had && renameFile(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
    if (renameFile(tmp.c_str(), path.c_str()) != 0) { if (had) renameFile(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    return true;
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

using Src = tr::InputSource;    // path-aware source identity / cache state (TapeReaderLogic.h)

struct TRState {
    std::string identity, root, symbol, mkt; int spb = 0; double tick = 0; std::string state = "starting"; bool faulted = false;
    std::vector<tr::Bar> bars; long nBars = -1; unsigned long long lastRaw = 0, firstRaw = 0;
    // inputs
    Src sDp, sOf[tr::NSRC], sLiqZ, sLiqSt, sKey, sMq, sTouch, sDealer, sHl, sTurns, sParams, sDealerSt, sDeltaSt;
    Src sOpt, sOfParams; ofo::History optHist; ofo::Snap optLast; std::string optWhy = "not read"; int gates[ofo::NGATE] = { 1, 1, 1, 1, 1, 1, 1 };   // (1.1.0) OPTIONS
    tr::DeltaSnap dp; tr::OfSnap of[tr::NSRC]; tr::LiqSnap liq; std::vector<tr::Level> key, mq; std::vector<tr::Touch> touch;
    long long keyAt = 0; bool mqStale = false; std::string mqSym; dl::Data dealer; long long dealerBook = tr::BOOK_ABSENT; tr::HodLodSnap hl; tr::TurnsSnap turns; tr::Params P;
    int profReach = 150, dltW = 50; bool dltLeft = false;
    // reads
    tr::Read live; bool haveLive = false; unsigned long long finalRaw = 0, pendingRaw = 0; long long pendingSince = -1; bool firstFinal = true;
    std::map<unsigned long long, tr::Read> logged; std::string logPath, logSession; long long logWrites = 0, logFails = 0, logLoaded = 0, logRejected = 0;
    std::vector<tr::Read> logQueue; long long logTried = -1;      // reads waiting for a good append (retried every 2 s)
    std::set<std::string> seen; long long intraUpdates = 0, recomputes = 0;
    // hover
    int hover = -1; std::vector<void*> hoverWnds; long long redraws = 0;
    // status / place
    std::string lastStatus; long long lastStatusMs = -1, statusWrites = 0; long long placeSig = -2, placeChecked = -1; int pos = tr::POS_DEFAULT;
    std::string layoutNote; tr::Box lastBox; tr::Box lastRegion; bool lastOk = false; int lastLines = 0, lastSpot = -1, lastAvoid = 0;   // (1.1.1) the spot used; tables * 1000 + candles avoided
    // (1.1.3) the banner: Position + Offset X / Y + background (place file), the stable width, the hysteresis, the hover, the board
    bp::Place place; bool bg = true; std::string placeWhy; bp::StableW banW; bp::Hyst hyst; bool inBanner = false; long long bannerHovers = 0;
    std::string chartKey; bb::Warn warn; bool warnSeen = false; bb::Beat lqBeat; tr::Box lastDetail; bool lastDetailOn = false;
    // (1.3.0) the Market State: every source's facts, the zone story, the odds, the snapshot for Claude (behind the scenes - nothing new drawn)
    std::map<std::string, mst::Facts> msF; zst::Story story; unsigned long long storyRaw = 0; long long snapMs = -1, snapWrites = 0;
    odt::Table odds; long long oddsChecked = -1; std::string oddsWhy = "not read"; std::string readWords, readOdds = "?";
};

class TapeReader : public cppExtension {
public:
    SS_SAFE_DIALOG(TapeReader, "TapeReader")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    TapeReader() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoadBody(void);                                       // (1.1.3) fills the dialog from the place file (never saves)
    virtual int parmsApplyBody(void) { savePos(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, trset::spec(), trIds(), (int)i, snap_); savePos(); return RTX_OK; }
    static ss::Ids& trIds() { static ss::Ids I; return I; }        // (1.3.1) the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
    struct SetStore { std::string mkt; ss::Store st; };
    std::vector<SetStore> stores_;                                     // (1.3.1) per market, per object
    ss::Store& storeFor(const std::string& mkt);
    ss::Snap snap_;                                                    // (1.1.3) the dialog as it opened (1.3.1: SettingsSpec.h)
    HostSlot<TRState> slot_;
    TRState* state(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }
    TRState* testState() { return slot_.get(this, false); }
    void pump();
    void identify(TRState& S);
    bool readBars(TRState& S, long n);
    void loadInputs(TRState& S, long long nowMs);
    bool check(Src& s, long long nowMs, long long every);
    tr::Inputs inputsFor(TRState& S, int k, std::vector<tr::Bar>& work);
    void finalize(TRState& S, int k);
    void marketState(TRState& S, int kClosed, long long nowMs);
    void intrabar(TRState& S, long n);
    void openLog(TRState& S, long long ctSec);
    void writeLog(TRState& S, const tr::Read& R);
    void flushLog(TRState& S);
    void writeStatus(TRState& S);
    void savePos();
    int position(TRState& S);
    int priceEdge();
    bool mouseLeft(TRState& S);
    void candleBoxes(const tr::Box& region, std::vector<tr::Box>& out);   // (1.1.1) the price candles the box keeps clear of
    bool visRange(unsigned long long& t0, unsigned long long& t1);   // (1.1.1) HoverBoard.h: the chart-wide hover
    bool followBoard(TRState& S);
    void publishBoard(TRState& S, long bar);
    int textW(const std::string& s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s.c_str(), -1); }
    void execute(const tr::Layout& L);
    long long nowCtSec() {   // (1.0.1) the wall clock on the bars' CT clock
#if defined(TR_TEST_CLOCK)
        return ctOf((unsigned long long)tr_test_unix);
#else
        return ctOf((unsigned long long)currentDate());
#endif
    }
    long long ctOf(unsigned long long raw) {
        struct tm t; std::memset(&t, 0, sizeof(t));
        if (!getLocaltime((RTDATE)raw, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
        return tr::civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
    }
};

int cppExtension::init(void)    { return RTX_OK; }
// (1.1.3) removed from the chart: its banner entry goes, so the Liquidity Profile draws its own warning again
static void trClearBoard(TapeReader* x) { TRState* S = x->testState(); if (S && !S->symbol.empty()) bb::clearBanner(S->symbol, S->spb, S->chartKey); }
int cppExtension::done(void)    { trClearBoard(static_cast<TapeReader*>(this)); static_cast<TapeReader*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { trClearBoard(static_cast<TapeReader*>(this)); static_cast<TapeReader*>(this)->clearState(); return RTX_OK; }
int cppExtension::setup(void)
{
    // controls only (Rassul 2026-10-02: settings windows show controls only). (1.1.3) Position (9 spots, default Top left), Offset X /
    // Y (px, -600..600), Banner background; version 2 = IRT resets the saved rows once (the chart reads the place file, the dialog is
    // filled from it when it opens)
    // (1.3.1, settings audit 2026-10-18) the same rows from trset::spec(), registered / filled / read by SettingsHost.h (no index arithmetic)
    ss::registerAll(this, trset::spec(), TapeReader::trIds());
    CPEN p(0x009CA3AF, 1, P_SOLID);
    setOutputParameter("Tape Reader", DRAW_INVISIBLE, &p, 0x009CA3AF, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}
int cppExtension::calc(int)
{
    TapeReader* x = static_cast<TapeReader*>(this);
    x->pump();
    TRState* S = x->testState();
    if (S) {
        bool ch = x->mouseLeft(*S);
        if (x->followBoard(*S)) ch = true;                                 // (1.1.1) the mouse over an order-flow pane of this chart
        if (ch) { ++S->redraws; x->invalidateChart(false); }
    }
    return RTX_OK;
}

void TapeReader::identify(TRState& S)
{
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!tr::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    tick = std::floor(tick * 1e9 + 0.5) / 1e9;
    const std::string id = root + "|" + sym + "|" + std::to_string(spb) + "|" + std::to_string(tick);
    if (id == S.identity) return;
    TRState fresh;                                                       // a symbol / period change: everything for this chart restarts
    fresh.identity = id; fresh.root = root; fresh.symbol = sym; fresh.spb = spb; fresh.tick = tick; fresh.mkt = dl::marketFor(0, root.empty() ? sym : root);
    fresh.pos = S.pos; fresh.placeSig = -2;
    S = fresh;
    if (S.mkt.empty()) S.state = "unknown market (root " + root + ")";
    else if (tick <= 0) S.state = "no tick size from IRT";
    else if (spb <= 0) S.state = "seconds per bar unknown";
}

bool TapeReader::readBars(TRState& S, long n)
{
    RTARRAYI dt(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if ((long)dt.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.state = "bar arrays unavailable"; return false; }
    const unsigned long long last = (unsigned long long)dt[(int)n - 1], first = (unsigned long long)dt[0];
    if (n == S.nBars && last == S.lastRaw && first == S.firstRaw && !S.bars.empty()) {   // same bars: only the forming bar moves
        tr::Bar& b = S.bars.back(); b.h = hi[(int)n - 1]; b.l = lo[(int)n - 1]; b.c = cl[(int)n - 1]; b.o = op[(int)n - 1];
        return true;
    }
    S.nBars = n; S.lastRaw = last; S.firstRaw = first;
    const long from = n > (long)KEEP_BARS ? n - (long)KEEP_BARS : 0;
    S.bars.clear(); S.bars.reserve((size_t)(n - from));
    for (long i = from; i < n; ++i) {
        tr::Bar b; b.raw = (unsigned long long)dt[(int)i]; b.t = ctOf(b.raw); b.o = op[(int)i]; b.h = hi[(int)i]; b.l = lo[(int)i]; b.c = cl[(int)i];
        b.v = (long)vo.count >= n ? (double)vo[(int)i] : 0;
        if (!tr::finite(b.h) || !tr::finite(b.l) || !tr::finite(b.c) || b.t <= 0) continue;
        S.bars.push_back(b);
    }
    return S.bars.size() >= 3;
}

// stat at most every `every` ms; true when the file changed (or appeared / vanished) since it was last read
bool TapeReader::check(Src& s, long long nowMs, long long every)
{
    return tr::checkInput(s, nowMs, every);
}

void TapeReader::loadInputs(TRState& S, long long nowMs)
{
    const std::string F = flexDir(); if (F.empty() || S.mkt.empty()) return;
    const std::string M = S.mkt, spb = std::to_string(S.spb);
    const long long ct = S.bars.empty() ? 0 : S.bars.back().t;
    // paths (session files follow the forming candle's session)
    S.sDp.path = F + "DeltaProfile" + SEP() + M + "-" + tr::contractToken(S.symbol) + "-" + spb + "-signals-" + std::to_string(tr::sessionDays(ct)) + ".csv";
    for (int k = 0; k < tr::NSRC; ++k) S.sOf[k].path = F + tr::SRC_NAME[k] + SEP() + M + "-" + tr::fileKey(S.symbol) + "-" + spb + "-" + tr::sessionYmd(ct) + ".csv";
    S.sLiqZ.path = F + "LRA-LiqZones-" + M + ".csv"; S.sLiqSt.path = F + "LiquidityProfile.status-" + M + "-" + spb + ".txt";
    S.sKey.path = F + "LRA-KeyLevels-" + M + ".csv"; S.sMq.path = F + "LRA-Levels-" + M + ".csv"; S.sTouch.path = F + "LRA-Touch-" + M + ".csv";
    S.sDealer.path = F + "LRA-Dealer-" + M + ".csv"; S.sHl.path = F + "HodLod.status-" + M + "-" + spb + ".txt";
    S.sTurns.path = F + "LRA-Turns-" + M + ".csv"; S.sParams.path = F + "LRA-TapeReader-" + M + ".csv";
    S.sDealerSt.path = F + "DealerProfile.status.txt"; S.sDeltaSt.path = F + "DeltaProfile.status.txt";
    S.sOpt.path = F + "LRA-Options-" + M + ".csv"; S.sOfParams.path = F + "LRA-OrderFlow-" + M + ".csv";   // (1.1.0)
    std::string text; long long sig;
    auto readInto = [&](Src& s, size_t maxB, bool tail, bool complete, std::vector<std::string>& L) -> bool {
        const int r = tr::readBounded(s.path, maxB, text, sig, tail);
        s.res = r;
        if (r == tr::RD_MISSING) { s.good = false; s.note = "missing"; return false; }
        if (r == tr::RD_TOOBIG) { s.good = false; s.note = "too large"; return false; }
        if (r == tr::RD_TORN) { s.sig = -2; s.note = "changed while read (kept last good)"; return false; }   // retried next check
        L = tr::lines(text, complete); return true;
    };
    std::vector<std::string> L;
    if (check(S.sDp, nowMs, CHECK_FAST_MS)) {
        if (readInto(S.sDp, 4u * 1024u * 1024u, true, true, L)) { tr::parseDelta(L, S.root, S.dp); S.sDp.good = true; S.sDp.note = std::to_string(S.dp.a.size()) + " A, " + std::to_string(S.dp.ae.size()) + " AE, " + std::to_string(S.dp.ex.size()) + " Ex, rejected " + std::to_string(S.dp.rejected); }
        else if (S.sDp.res != tr::RD_TORN) S.dp = tr::DeltaSnap();
    }
    for (int k = 0; k < tr::NSRC; ++k) if (check(S.sOf[k], nowMs, CHECK_FAST_MS)) {
        if (readInto(S.sOf[k], 6u * 1024u * 1024u, false, false, L)) { tr::parseOf(L, S.of[k]); S.sOf[k].good = S.of[k].header; S.sOf[k].note = S.of[k].header ? std::to_string(S.of[k].rows.size()) + " candles, rejected " + std::to_string(S.of[k].rejected) : "no header line"; }
        else if (S.sOf[k].res != tr::RD_TORN) S.of[k] = tr::OfSnap();
    }
    if (check(S.sLiqZ, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sLiqZ, 64 * 1024, false, false, L)) { tr::LiqSnap q; if (tr::parseLiqZones(L, q)) { q.lq = S.liq.lq; q.sym = S.liq.sym; S.liq = q; S.sLiqZ.good = true; S.sLiqZ.note = std::to_string(q.z.size()) + " zones"; } else { S.sLiqZ.good = false; S.sLiqZ.note = "garbled"; } }
    }
    if (check(S.sLiqSt, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sLiqSt, 256 * 1024, false, false, L)) { tr::parseLiqStatus(L, S.liq); S.sLiqSt.good = true; S.sLiqSt.note = std::to_string(S.liq.lq.size()) + " LQ"; }
    }
    if (check(S.sKey, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sKey, 256 * 1024, false, false, L)) { std::vector<tr::Level> v; long long at; if (tr::parseKeyLevels(L, at, v)) { S.key = v; S.keyAt = at; S.sKey.good = true; S.sKey.note = std::to_string(v.size()) + " levels"; } else { S.sKey.good = false; S.sKey.note = "garbled"; } }
        else if (S.sKey.res != tr::RD_TORN) S.key.clear();
    }
    if (check(S.sMq, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sMq, 256 * 1024, false, false, L)) { tr::parseMqLevels(L, S.mq, S.mqStale, S.mqSym); S.sMq.good = true; S.sMq.note = std::to_string(S.mq.size()) + " levels" + (S.mqStale ? ", STATE STALE" : ""); }
        else if (S.sMq.res != tr::RD_TORN) S.mq.clear();
    }
    if (check(S.sTouch, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sTouch, 256 * 1024, false, false, L)) { tr::parseTouch(L, S.touch); S.sTouch.good = true; S.sTouch.note = std::to_string(S.touch.size()) + " rows"; }
        else if (S.sTouch.res != tr::RD_TORN) S.touch.clear();
    }
    if (check(S.sDealer, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sDealer, 512 * 1024, false, false, L)) { dl::Data D; for (size_t i = 0; i < L.size(); ++i) dl::parseLine(D, L[i]); S.dealer = D; S.dealerBook = tr::parseBookAsof(L); S.sDealer.good = true; S.sDealer.note = std::to_string(D.nodes.size()) + " nodes" + (D.chartSym.empty() ? "" : ", SYMBOL " + D.chartSym); }
        else if (S.sDealer.res != tr::RD_TORN) { S.dealer = dl::Data(); S.dealerBook = tr::BOOK_ABSENT; }
    }
    if (check(S.sHl, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sHl, 256 * 1024, false, false, L)) { tr::parseHodLod(L, S.hl); S.sHl.good = S.hl.ok; S.sHl.note = S.hl.ok ? "FIRST " + (S.hl.first.empty() ? std::string("not clear") : S.hl.first) : "no FIRST line"; }
        else if (S.sHl.res != tr::RD_TORN) S.hl = tr::HodLodSnap();
    }
    if (check(S.sTurns, nowMs, 10000)) {
        const bool ok = readInto(S.sTurns, 256 * 1024, false, false, L);
        if (ok || S.sTurns.res == tr::RD_MISSING) { S.turns = tr::parseTurns(ok, ok ? L : std::vector<std::string>(), S.mkt); S.sTurns.good = S.turns.ok; S.sTurns.note = S.turns.why; }
    }
    if (check(S.sParams, nowMs, 10000)) {
        const bool ok = readInto(S.sParams, 64 * 1024, false, false, L);
        if (ok || S.sParams.res == tr::RD_MISSING) { S.P = tr::parseParams(ok, ok ? L : std::vector<std::string>(), S.mkt); S.sParams.good = S.P.src == "file"; S.sParams.note = S.P.src + (S.P.why.empty() ? "" : ": " + S.P.why); }
    }
    // (1.1.0) OPTIONS: the bridge's file after each MenthorQ snapshot; a new grid joins the history on the bar it ARRIVED on
    if (check(S.sOpt, nowMs, CHECK_SLOW_MS)) {
        if (readInto(S.sOpt, 16 * 1024, false, false, L)) {
            S.optLast = ofo::parse(L, S.mkt); S.sOpt.good = S.optLast.ok;
            S.optWhy = S.optLast.ok ? "ok" : (S.optLast.notes.empty() ? std::string("garbled") : S.optLast.notes[0]);
            if (S.optLast.ok && !S.bars.empty()) {                       // arrived on the bar during which the file was written (its mtime)
                long long nowCt = nowCtSec(); if (nowCt <= 0) nowCt = S.bars.back().t;
                const long long target = nowCt - (wallNow() - S.sOpt.mtime); unsigned long long raw = S.bars.back().raw;
                for (size_t i = S.bars.size(); i-- > 0;) { if (S.bars[i].t < target) break; raw = S.bars[i].raw; }
                S.optHist.add(S.optLast, (long long)raw);
            }
            S.sOpt.note = S.optWhy + (S.optLast.ok ? ", grid " + std::to_string(S.optLast.asof) : std::string());
        } else if (S.sOpt.res != tr::RD_TORN) { S.optLast = ofo::Snap(); S.optWhy = S.sOpt.note; }
    }
    if (check(S.sOfParams, nowMs, 10000)) {
        if (readInto(S.sOfParams, 64 * 1024, false, false, L)) { ofo::parseGates(L, S.gates); S.sOfParams.good = true; S.sOfParams.note = "gates read"; }
        else if (S.sOfParams.res == tr::RD_MISSING) { for (int g = 0; g < ofo::NGATE; ++g) S.gates[g] = ofo::GATE_Q; }
    }
    // the profiles' widths (the box never reaches them)
    if (check(S.sDealerSt, nowMs, CHECK_SLOW_MS) && readInto(S.sDealerSt, 256 * 1024, false, false, L)) {
        for (size_t i = 0; i < L.size(); ++i) if (L[i].compare(0, 6, "REACH,") == 0) { long long r; if (tr::parseWhole(L[i].substr(6), 40, 1500, r)) S.profReach = (int)r; }
    }
    if (check(S.sDeltaSt, nowMs, CHECK_SLOW_MS) && readInto(S.sDeltaSt, 256 * 1024, false, false, L)) {
        for (size_t i = 0; i < L.size(); ++i) {
            if (L[i].compare(0, 6, "WIDTH,") == 0) { long long w; if (tr::parseWhole(L[i].substr(6), 20, 600, w)) S.dltW = (int)w; }
            if (L[i].compare(0, 6, "PLACE,") == 0) S.dltLeft = L[i].substr(6) == "Left";
        }
    }
}

// the inputs of candle k (work = the bars up to k; the forming bar intrabar). Stale inputs are left out; ages go to the log.
tr::Inputs TapeReader::inputsFor(TRState& S, int k, std::vector<tr::Bar>& work)
{
    tr::Inputs I; I.mkt = S.mkt; I.root = S.root; I.tick = S.tick; I.spb = S.spb; I.P = S.P;
    work.assign(S.bars.begin(), S.bars.begin() + (k + 1));
    I.B = &work; I.k = k;
    const long long now = wallNow();
    std::string in;
    auto age = [&](const Src& s) { return s.mtime > 0 ? std::to_string(std::max(0LL, now - s.mtime)) + "s" : std::string("-"); };
    auto note = [&](const char* name, bool used, const Src& s, const char* why) { in += std::string(in.empty() ? "" : ";") + name + ":" + (used ? age(s) : std::string(why)); };
    // Delta + order flow: session records, as-of filtered by time inside compute()
    if (S.sDp.good) I.dp = &S.dp;
    note("delta", S.sDp.good, S.sDp, S.sDp.note == "missing" ? "missing" : "bad");
    for (int s = 0; s < tr::NSRC; ++s) { if (S.sOf[s].good) I.of[s] = &S.of[s];
        note(s == tr::SRC_BAS ? "bas" : s == tr::SRC_PULL ? "pull" : "stack", S.sOf[s].good, S.sOf[s], S.sOf[s].note == "missing" ? "missing" : "bad"); }
    // liquidity: fresh within 60 s, same contract
    const bool liqFresh = S.sLiqZ.good && S.liq.at > 0 && now - S.liq.at <= 60 && S.liq.at - now <= 60 && (S.liq.sym.empty() || dl::sameContract(S.symbol, S.liq.sym));
    if (liqFresh) I.liq = &S.liq;
    note("liq", liqFresh, S.sLiqZ, S.sLiqZ.good ? "stale" : "missing");
    // RTH (CT minutes) first: the GAMMA freshness rule needs it
    int openMin = 0, closeMin = 0;
    const bool haveRth = svl::rthOf(S.mkt.c_str(), openMin, closeMin);
    // (1.0.1) the Dealer file is judged by its DATA time (ASOF / the index book's SRC time), not its save time
    long long nowCt = nowCtSec();
    if (nowCt <= 0 && !S.bars.empty()) nowCt = S.bars.back().t;
    const long long dataCt = S.sDealer.good ? tr::dealerDataCt(S.dealer.y, S.dealer.mo, S.dealer.d, S.dealer.asofSo, S.dealer.srcAt, S.dealerBook) : -1;
    const bool dealerFresh = S.sDealer.good && dataCt >= 0 && nowCt > 0 && nowCt - dataCt <= tr::GAMMA_OFF_MAX_S;
    // levels: key (<= 18 h), MenthorQ (<= 18 h, same contract as the Dealer file), touch odds
    const bool contractOk = S.dealer.chartSym.empty() || dl::sameContract(S.symbol, S.dealer.chartSym);
    I.roll = dealerFresh && !contractOk;
    const bool keyFresh = S.sKey.good && now - S.keyAt <= 18 * 3600 && S.keyAt - now <= 600;
    const bool mqFresh = S.sMq.good && !S.mqStale && now - S.sMq.mtime <= 18 * 3600 && contractOk && (S.mqSym.empty() || dl::sameContract(S.symbol, S.mqSym));
    const bool touchFresh = S.sTouch.good && now - S.sTouch.mtime <= 18 * 3600 && contractOk;
    if (mqFresh) I.mqRaw = S.mq;
    I.levels = tr::mergeLevels(keyFresh ? S.key : std::vector<tr::Level>(), mqFresh ? S.mq : std::vector<tr::Level>(), touchFresh ? S.touch : std::vector<tr::Touch>(), S.tick);
    note("keylv", keyFresh, S.sKey, S.sKey.good ? "stale" : "missing"); note("mqlv", mqFresh, S.sMq, !contractOk ? "other contract" : S.sMq.good ? "stale" : "missing");
    note("touch", touchFresh, S.sTouch, S.sTouch.good ? "stale" : "missing");
    {   // GAMMA feed: g, else MenthorQ net GEX $ (index book in cash hours), else abstain; stale data abstains
        std::vector<tr::DealerNodeIn> rows;
        for (const dl::Node& n : S.dealer.nodes) { tr::DealerNodeIn r; r.px = n.k; r.g = n.g; r.usd = n.usd; r.hasUsd = n.hasUsd; rows.push_back(r); }
        const tr::GammaFeed GF = tr::gammaFeed(S.sDealer.good && contractOk, rows, dataCt, nowCt, haveRth ? openMin : -1, haveRth ? closeMin : -1);
        I.gammaState = GF.state; I.gammaAgeS = GF.ageS; I.gnodes = GF.nodes;
        const std::string why = !S.sDealer.good ? "missing" : !contractOk ? "other contract" : dataCt < 0 ? (S.dealerBook == tr::BOOK_NONE ? "no book time" : "no data time") :
                                GF.state == tr::GF_STALE ? "stale data " + tr::ageText(GF.ageS) : GF.state == tr::GF_NONE ? "no gamma" : "";
        in += std::string(in.empty() ? "" : ";") + "dealer:" + (GF.state == tr::GF_OK ? std::to_string(GF.ageS) + "s" + (GF.fromUsd ? " gex$" : "") : why);
    }
    const bool hlFresh = S.sHl.good && now - S.sHl.mtime <= 120;
    if (hlFresh) I.hl = &S.hl;
    note("hodlod", hlFresh, S.sHl, S.sHl.good ? "stale" : "missing");
    const bool turnsFresh = S.turns.ok && now - S.sTurns.mtime <= 72 * 3600;
    if (turnsFresh) I.turns = &S.turns;
    note("turns", turnsFresh, S.sTurns, S.turns.ok ? "stale" : "learning");
    // trend: native (VwapTrendLogic.h, the rule lsSessionVWAP publishes), from the bars up to k
    if (haveRth) {
        I.rthOpen = openMin; I.rthClose = closeMin;
        std::vector<svl::Bar> vb; vb.reserve(work.size());
        for (const tr::Bar& b : work) { svl::Bar x; x.days = (long)(b.t >= 0 ? b.t / 86400 : -1); x.endSec = (int)(b.t % 86400); x.h = (float)b.h; x.l = (float)b.l; x.c = (float)b.c; x.v = b.v; vb.push_back(x); }
        const vtl::Trend T = vtl::fromChart(vb, (int)vb.size(), S.spb, openMin, closeMin);
        I.trend = vtl::level(T); I.trendReady = T.ready;
    }
    note("trend", I.trendReady, Src(), "not ready");
    {   // (1.1.0) the options grid that had ARRIVED by candle k, fresh at its end (unix = the wall clock minus the chart-clock gap)
        const ofo::Snap* os = S.optHist.latest((long long)S.bars[(size_t)k].raw);
        const long long ref = nowCt > 0 ? now - (nowCt - S.bars[(size_t)k].t) : now;
        const bool fresh = os && ofo::fresh(*os, ref);
        if (fresh) { I.opt = os; I.optAgeS = std::max(0LL, ref - os->asof); }   // (1.2.0) the grid's age for "(model, 7m old)"
        for (int g = 0; g < ofo::NGATE; ++g) I.optGates[g] = S.gates[g];
        in += std::string(";options:") + (fresh ? std::to_string(std::max(0LL, ref - os->asof)) + "s" : os ? std::string("stale") : S.sOpt.good ? std::string("not arrived") : std::string("missing"));
    }
    I.inputs = in;
    return I;
}

void TapeReader::openLog(TRState& S, long long ctSec)
{
    const std::string ss = tr::sessionYmd(ctSec);
    if (ss == S.logSession && !S.logPath.empty()) return;
    S.logSession = ss; S.logged.clear(); S.logLoaded = S.logRejected = 0;
    const std::string F = flexDir(); if (F.empty()) { S.logPath.clear(); return; }
    S.logPath = F + "TapeReader" + SEP() + S.mkt + "-" + tr::fileKey(S.symbol) + "-" + std::to_string(S.spb) + "-" + ss + ".csv";
    std::string text; long long sig;
    if (tr::readBounded(S.logPath, 8u * 1024u * 1024u, text, sig, true) == tr::RD_OK) {
        const std::vector<std::string> L = tr::lines(text, true);
        for (size_t i = 0; i < L.size(); ++i) {
            if (L[i].empty() || L[i][0] == '#' || L[i].compare(0, 8, "bar_raw|") == 0) continue;
            tr::Read r; if (tr::parseLogLine(L[i], r)) { S.logged[r.raw] = r; ++S.logLoaded; } else ++S.logRejected;
        }
    }
}
// checklist #6: a candle is marked logged only after a flushed, good append; a failed append stays queued and is retried
void TapeReader::writeLog(TRState& S, const tr::Read& R)
{
    openLog(S, R.t);
    if (S.logPath.empty() || S.logged.count(R.raw)) return;
    for (const tr::Read& q : S.logQueue) if (q.raw == R.raw) return;
    S.logQueue.push_back(R);
    while (S.logQueue.size() > 50) S.logQueue.erase(S.logQueue.begin());
    flushLog(S);
}
void TapeReader::flushLog(TRState& S)
{
    S.logTried = steadyMs();
    while (!S.logQueue.empty()) {
        const tr::Read R = S.logQueue.front();
        openLog(S, R.t);
        if (S.logPath.empty()) return;
        if (S.logged.count(R.raw)) { S.logQueue.erase(S.logQueue.begin()); continue; }
        makeDir(flexDir() + "TapeReader");
        const bool fresh = tr::fileSig(S.logPath) < 0;
        std::ofstream f(S.logPath.c_str(), std::ios::binary | std::ios::app);
        if (!f) { ++S.logFails; return; }
        if (fresh) f << "# lsTapeReader " << TR_VERSION << " " << S.mkt << " " << S.symbol << " " << S.spb << "\n" << tr::LOG_HEADER << "\n";
        f << tr::logLine(R, S.P) << "\n"; f.flush();
        if (!f) { ++S.logFails; return; }
        S.logged[R.raw] = R; ++S.logWrites; S.logQueue.erase(S.logQueue.begin());
        while (S.logged.size() > 6000) S.logged.erase(S.logged.begin());
    }
}

// the candle's read, finalised once: shown live, logged (not the first one after a load: its inputs are "now", not the candle's)
void TapeReader::finalize(TRState& S, int k)
{
    std::vector<tr::Bar> work; tr::Inputs I = inputsFor(S, k, work);
    tr::Read R = tr::compute(I); ++S.recomputes;
    if (!R.ok) return;
    R.intraUpdates = (int)S.intraUpdates; S.intraUpdates = 0;
    S.live = R; S.haveLive = true;
    if (!S.firstFinal) writeLog(S, R);
    S.firstFinal = false;
    // the events known by this candle's end never trigger an intrabar update later
    for (const tr::DeltaA& a : S.dp.a) if (a.decidedBy <= R.t) S.seen.insert("A" + std::to_string(a.id));
    for (const tr::DeltaAE& e : S.dp.ae) if (e.decT <= R.t) S.seen.insert("AE" + std::to_string(e.aId));
}

// intrabar: only a NEW A / AE (Delta Profile) or S / R (an order-flow row of the forming candle) at a level recomputes
void TapeReader::intrabar(TRState& S, long n)
{
    if (!S.haveLive || S.bars.size() < 3) return;
    const tr::Bar& fb = S.bars.back(); const long long tPrev = S.bars[S.bars.size() - 2].t;
    std::vector<std::string> fresh;
    std::vector<tr::Level> lv = tr::mergeLevels(S.key, S.mq, S.touch, S.tick);
    const double band = std::max(3.0 * S.tick, 0.5 * tr::atrOf(S.bars, (int)S.bars.size() - 2));
    auto atLevel = [&](double px) { for (const tr::Level& v : lv) if (std::fabs(v.px - px) <= band) return true; return false; };
    for (const tr::DeltaA& a : S.dp.a) if (a.decidedBy > tPrev && !S.seen.count("A" + std::to_string(a.id)) && atLevel(a.support ? a.lo : a.hi)) fresh.push_back("A" + std::to_string(a.id));
    for (const tr::DeltaAE& e : S.dp.ae) if (e.decT > tPrev && !S.seen.count("AE" + std::to_string(e.aId))) fresh.push_back("AE" + std::to_string(e.aId));
    for (int s = 0; s < tr::NSRC; ++s) {
        std::map<unsigned long long, tr::OfRow>::const_iterator it = S.of[s].rows.find(fb.raw);
        if (it == S.of[s].rows.end() || it->second.level.empty()) continue;
        for (int side = 0; side < 2; ++side) for (char ch : it->second.let[side]) if (ch == 'S' || ch == 'I') {   // (1.0.1) I = refill / iceberg (parse maps legacy R to I)
            const std::string key = std::string(tr::SRC_NAME[s]) + std::to_string(fb.raw) + "|" + std::to_string(side) + ch;
            if (!S.seen.count(key)) fresh.push_back(key);
        }
    }
    if (fresh.empty()) return;
    for (const std::string& f : fresh) S.seen.insert(f);
    std::vector<tr::Bar> work; tr::Inputs I = inputsFor(S, (int)S.bars.size() - 1, work);
    tr::Read R = tr::compute(I); ++S.recomputes;
    if (!R.ok) return;
    R.intrabar = true; ++S.intraUpdates; R.intraUpdates = (int)S.intraUpdates;
    S.live = R;
    (void)n;
    ++S.redraws; invalidateChart(false);
}

void TapeReader::pump()
{
    TRState* P = state(true);
    if (!P) return;
    TRState& S = *P;
    try {
        identify(S);
        if (S.mkt.empty() || S.tick <= 0 || S.spb <= 0 || S.faulted) return;
        const long n = getBarCount();
        if (n < 4) { S.state = "too few bars"; return; }
        if (!readBars(S, n)) return;
        const long long now = steadyMs();
        loadInputs(S, now);
        if (S.bars.size() < 4) { S.state = "too few bars"; return; }
        const int kClosed = (int)S.bars.size() - 2;
        const unsigned long long raw = S.bars[(size_t)kClosed].raw;
        openLog(S, S.bars.back().t);
        if (!S.logQueue.empty() && (now - S.logTried >= 2000 || now < S.logTried)) flushLog(S);
        if (raw != S.finalRaw && raw != S.pendingRaw) { S.pendingRaw = raw; S.pendingSince = now; }
        if (S.pendingRaw != 0 && S.pendingRaw == raw) {
            bool complete = true;                                        // every active order-flow pane has logged this candle
            for (int s = 0; s < tr::NSRC; ++s) if (S.sOf[s].good && wallNow() - S.sOf[s].mtime <= ACTIVE_S && !S.of[s].rows.count(raw)) complete = false;
            if (complete || S.firstFinal || now - S.pendingSince >= GRACE_MS || now < S.pendingSince) {
                finalize(S, kClosed); S.finalRaw = raw; S.pendingRaw = 0; ++S.redraws; invalidateChart(false);
            }
        } else intrabar(S, n);
        marketState(S, kClosed, now);
        S.state = S.haveLive ? "ready" : "waiting for the first candle close";
    } catch (...) { S.faulted = true; S.state = "calculation error (cleared on symbol change)"; }
}

// (1.3.1) TapeReader.settings-<MKT>.txt (keyed), migrated once from TapeReader.place-<MKT>.txt ("pos,dx,dy,bg"), which is left in place
static std::string trMarket(cppExtension* x) { char rb[32]; std::memset(rb, 0, sizeof(rb)); const char* rs = x->getRootSymbol(rb); return dl::marketFor(0, rs ? rs : ""); }
ss::Store& TapeReader::storeFor(const std::string& mkt)
{
    for (size_t i = 0; i < stores_.size(); i++) if (stores_[i].mkt == mkt) return stores_[i].st;
    if (stores_.size() >= 8) stores_.erase(stores_.begin());
    const std::string F = flexDir();
    SetStore n; n.mkt = mkt; n.st.S = &trset::spec();
    if (!F.empty() && !mkt.empty()) { n.st.path = F + "TapeReader.settings-" + mkt + ".txt"; n.st.legacyPaths.push_back(F + "TapeReader.place-" + mkt + ".txt"); }
    n.st.legacy = trset::legacy;
    stores_.push_back(n); return stores_.back().st;
}
int TapeReader::parmsLoadBody(void)
{
    if (!ss::inDialog(this)) return RTX_OK;                              // opening never saves
    const std::string mkt = trMarket(this); if (mkt.empty()) return RTX_OK;
    ss::fillDialog(this, trset::spec(), trIds(), storeFor(mkt).load(ss::marketIndex(mkt), steadyMs(), true), snap_);
    return RTX_OK;
}
void TapeReader::savePos()
{
    if (!ss::inDialog(this)) return;                                    // (1.1.3) only the settings window's values are real
    const std::string mkt = trMarket(this); if (mkt.empty()) return;
    ss::Store& st = storeFor(mkt);
    const ss::Values F = st.load(ss::marketIndex(mkt), steadyMs(), true);
    const ss::Values V = ss::readDialog(this, trset::spec(), trIds(), F, snap_);   // untouched -> the file's value; garbage never saved
    if (st.save(V)) snap_.take(ss::dialogNow(this, trset::spec(), trIds(), V));
    TRState* S = testState(); if (S) { S->placeSig = -2; S->placeChecked = -1; }
}
// the per-market settings file is the only source while drawing (IRT hands the chart copy list 0): no file = Top left (1.1.3)
int TapeReader::position(TRState& S)
{
    const long long now = steadyMs();
    if (S.placeChecked >= 0 && now >= S.placeChecked && now - S.placeChecked < 1000) return S.pos;
    S.placeChecked = now;
    if (flexDir().empty() || S.mkt.empty()) { S.place = bp::Place(); S.place.pos = tr::POS_DEFAULT; S.bg = true; return S.pos = tr::POS_DEFAULT; }
    const ss::Values V = storeFor(S.mkt).load(ss::marketIndex(S.mkt), now);
    bp::Place pl; pl.pos = V.i(trset::K_POSITION); pl.dx = 0; pl.dy = 0;   // (1.3.2) the 9-spot Position only
    if (pl.pos != S.place.pos || pl.dx != S.place.dx || pl.dy != S.place.dy) { S.hyst.reset(); S.inBanner = false; }   // moved: start clean
    S.place = pl; S.bg = V.b(trset::K_BACKGROUND); S.placeWhy = V.why; S.pos = pl.pos;
    return S.pos;
}
// the last price bar's right edge (x): the usable area ends there (never beyond price); -1 = unknown
int TapeReader::priceEdge()
{
    const long n = getBarCount(); if (n < 2) return -1;
    RTARRAY cl(barClose); if ((long)cl.count < n) return -1;
    PNT p; if (p.set((int)(n - 1), cl[(int)(n - 1)], kBarRight) != RTX_OK) return -1;
    return p.h > 0 ? (int)p.h : -1;
}

// (1.1.1) the visible candles inside the region as boxes (high to low, the bar's width), at most 400 - the "soft" area to keep clear
void TapeReader::candleBoxes(const tr::Box& region, std::vector<tr::Box>& out)
{
    out.clear(); int a = 0, b = 0; const long n = getBarCount();
    if (n < 1 || getVisibleBars(&a, &b) != RTX_OK) return;
    RTARRAY hi(barHigh), lo(barLow); if ((long)hi.count < n || (long)lo.count < n) return;
    if (a < 0) a = 0;
    if (b > (int)n - 1) b = (int)n - 1;
    if (b - a > 400) a = b - 400;
    for (int i = a; i <= b; ++i) {
        PNT L, R; if (L.set(i, hi[i], kBarLeft) != RTX_OK || R.set(i, lo[i], kBarRight) != RTX_OK) continue;
        tr::Box c; c.l = std::min((int)L.h, (int)R.h); c.r = std::max((int)L.h, (int)R.h) + 1; c.t = std::min((int)L.v, (int)R.v); c.b = std::max((int)L.v, (int)R.v) + 1;
        if (c.r < region.l || c.l > region.r) continue;
        out.push_back(c);
    }
}
int TapeReader::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    TRState* S = testState(); if (!S) return RTX_FAIL;
    if (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER) return RTX_FAIL;
    void* w = cursorWindow();
    if (w) {
        std::vector<void*>::iterator it = std::find(S->hoverWnds.begin(), S->hoverWnds.end(), w);
        if (it != S->hoverWnds.end()) S->hoverWnds.erase(it);
        S->hoverWnds.push_back(w);
        if (S->hoverWnds.size() > MAX_HOVER_WNDS) S->hoverWnds.erase(S->hoverWnds.begin());
    }
    {   // (1.1.3) over the banner (or its open detail): the detail shows, the candle it reads stays
        const tr::Box& bx = S->lastBox; const tr::Box& dt = S->lastDetail; const int mh = e->v.mouse.h, mv = e->v.mouse.v;
        // a dead band (tr::HOVER_BAND px): it opens only inside the banner, and closes only once the mouse is that far outside the
        // banner / its detail - no flicker on the edge or on the 2 px gap between them
        const int g = S->inBanner ? tr::HOVER_BAND : 0;
        const bool in = TR_BANNER_HOVER && S->lastOk && (tr::inBox(bx, mh, mv, g) || (S->inBanner && S->lastDetailOn && tr::inBox(dt, mh, mv, g)));   // (1.3.3) TR_BANNER_HOVER
        if (in != S->inBanner) { S->inBanner = in; if (in) ++S->bannerHovers; ++S->redraws; invalidateChart(false); }
        if (in) return RTX_FAIL;
    }
    const long cnt = getBarCount(); const long b = e->v.mouse.bar;
    const int h = b >= 0 && b < cnt - 1 ? (int)b : -1;                   // a closed bar; the forming bar / past the last = live
    publishBoard(*S, b);                                                 // (1.1.1) the order-flow panes follow this candle
    if (h != S->hover) { S->hover = h; ++S->redraws; invalidateChart(false); }
    return RTX_FAIL;                                                     // never swallows the event
}
bool TapeReader::visRange(unsigned long long& t0, unsigned long long& t1)
{
    t0 = t1 = 0; int a = 0, b = 0; const long n = getBarCount();
    if (n < 1 || getVisibleBars(&a, &b) != RTX_OK) return false;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return false;
    if (a < 0) a = 0;
    if (b > (int)n - 1) b = (int)n - 1;
    if (b < a) return false;
    t0 = (unsigned long long)times[a]; t1 = (unsigned long long)times[b]; return t0 > 0;
}
bool TapeReader::followBoard(TRState& S)
{
    unsigned long long t0 = 0, t1 = 0, t = 0; if (S.symbol.empty() || !visRange(t0, t1) || !hb::hovered(S.symbol, S.spb, t0, t1, t)) return false;
    RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return false;
    const int h = hb::indexOf(times, n, t);
    if (h == S.hover) return false;
    S.hover = h; return true;
}
void TapeReader::publishBoard(TRState& S, long bar)
{
    unsigned long long t0 = 0, t1 = 0; if (S.symbol.empty() || !visRange(t0, t1)) return;
    RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return;
    hb::publish(S.symbol, S.spb, bar >= 0 && bar < n - 1 ? (unsigned long long)times[(int)bar] : 0ULL, t0, t1);
}
bool TapeReader::mouseLeft(TRState& S)
{
    if ((S.hover < 0 && !S.inBanner) || S.hoverWnds.empty()) return false;
    void* w = cursorWindow(); if (!w) return false;
    if (std::find(S.hoverWnds.begin(), S.hoverWnds.end(), w) != S.hoverWnds.end()) return false;
    S.hover = -1; S.inBanner = false; return true;                       // (1.1.3) off the chart: no banner hover either
}

void TapeReader::execute(const tr::Layout& L)
{
    for (size_t i = 0; i < L.items.size(); ++i) {
        const tr::Item& it = L.items[i];
        if (it.k == tr::K_RECT && it.c2 == 0xFFFFFFFFu) {                // (1.1.3) hollow (the warning tag): four solid lines, no fill
            setPen((COLOR)it.c1, 1, P_SOLID);
            PNT a; a.h = coord(it.l); a.v = coord(it.t); a.setDrawPosition(); a.h = coord(it.r); a.drawLineTo(); a.v = coord(it.b); a.drawLineTo(); a.h = coord(it.l); a.drawLineTo(); a.v = coord(it.t); a.drawLineTo();
        }
        else if (it.k == tr::K_RECT) { setPen((COLOR)it.c1, 1, P_SOLID); RCT rc; rc.set(coord(it.l), coord(it.t), coord(it.r), coord(it.b)); rc.draw(1, (COLOR)it.c1, (COLOR)it.c2, DRAW_OPAQUE, PAT_SOLID); }
        else if (it.k == tr::K_TEXT) {
            FONT f; f.id = HELVETICA; f.size = (short)it.fs; f.style = it.bold ? BOLD : PLAIN; setFont(f); setTextColor((COLOR)it.c1);
            const int yc = (it.t + it.b) / 2; const short yy = (short)(yc - (short)(it.fs * 0.45f + 0.5f));
            RCT rc; rc.set(coord(it.l), coord(yy - it.fs), coord(it.r + 4), coord(yy + it.fs)); rc.drawText(it.s.c_str(), false, false);
        }
    }
}

int TapeReader::draw(void)
{
    pump();
    TRState* S = testState();
    if (!S) return RTX_OK;
    mouseLeft(*S); followBoard(*S);                                     // (1.1.1) the candle under the mouse ANYWHERE on this chart
    try {
        RCT pane; pane.getPaneRect(false);
        tr::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
        // the region: the pane's left (clear of the Liquidity Profile column / a left-placed Delta Profile) to the last bar,
        // and never into the Dealer / Delta Profiles on the right
        const int W = pane.right - pane.left;
        int left = pane.left + (S->liq.colRight > 0 && S->sLiqZ.good ? S->liq.colRight + 6 : (W >= 600 ? 110 : 2));
        if (S->dltLeft) left = std::max(left, (int)pane.left + S->dltW + 110);
        int right = pane.right - S->profReach - 8 - (S->dltLeft ? 0 : S->dltW + 110);
        const int pe = priceEdge();
        if (pe > 0) right = std::min(right, pe);
        tr::Box region; region.l = left; region.t = pane.top + 2; region.r = right; region.b = pane.bottom - 2;
        S->lastRegion = region;
        if (S->mkt.empty() || !S->haveLive) {
            const std::string s = S->mkt.empty() ? trt::text("note.nomarket") : "Tape Reader: " + S->state;
            if (region.r - region.l > 160 && region.b - region.t > 30) { FONT f; f.id = HELVETICA; f.size = 8; f.style = PLAIN; setFont(f); setTextColor(tr::C_QUIET);
                RCT rc; rc.set(coord(region.l + 4), coord(region.b - 18), coord(region.l + 4 + textW(s, 8, false) + 4), coord(region.b - 4)); rc.drawText(s.c_str(), false, false); }
            writeStatus(*S); return RTX_OK;
        }
        const tr::Read* R = &S->live;
        if (S->hover >= 0 && S->hover < (int)getBarCount() - 1) {
            RTARRAYI dt(barDateTime);
            if ((long)dt.count > S->hover) { std::map<unsigned long long, tr::Read>::const_iterator it = S->logged.find((unsigned long long)dt[S->hover]); if (it != S->logged.end()) R = &it->second; }
        }
        // (1.1.3) the banner: "Tape Reader v1.1.3  .  7:09 AM" (the market is the chart's own) + the read; line 2 the Liquidity warning
        const std::string header = std::string("Tape Reader v") + TR_VERSION + "  \xB7  " + R->ct;
        { const char* cl = getChartLabel(); S->chartKey = cl ? std::string(cl) : std::string(); }
        const long long nowMs = steadyMs();
        {   // (CR5000) the Liquidity Profile's heartbeat (BannerBoard 1.0.1): present while its stamp moves, or under 5 s old, or for 3 of
            // my draws without a new stamp - an idle chart's first repaint never drops line 2 for a frame
            bb::Warn w; long long ms = -1; const bool have = bb::peekWarn(S->symbol, S->spb, S->chartKey, w, ms);
            S->warnSeen = S->lqBeat.step(have, ms, nowMs); S->warn = S->warnSeen ? w : bb::Warn();
        }
        // the area: the pane from its left edge (where the Liquidity warning sits today) to the last bar, below the IRT title row
        tr::Box area; area.l = pane.left + 4; area.t = pane.top + tr::TITLE_ROW; area.r = right; area.b = pane.bottom - 2;
        // never over the HOD / LOD table (its own status file, when fresh)
        std::vector<tr::Box> hard;
        if (S->hl.boxOk && S->sHl.good && wallNow() - S->sHl.mtime <= 120) { tr::Box hb; hb.l = S->hl.box[0] - 4; hb.t = S->hl.box[1] - 4; hb.r = S->hl.box[2] + 4; hb.b = S->hl.box[3] + 4; hard.push_back(hb); }
        position(*S);
        const int need = tr::bannerNeed(*R, header, S->warn, M);
        const int stableW = S->banW.step(need, nowMs);
        const tr::Banner Bn = tr::layoutBanner(*R, header, S->warn, area, S->place, stableW, S->bg, S->inBanner, M, hard, &S->hyst);
        S->lastOk = Bn.L.ok; S->lastBox = Bn.box; S->layoutNote = Bn.L.ok ? std::string("banner") : Bn.L.note; S->lastLines = Bn.L.lines; S->lastSpot = S->place.pos; S->lastAvoid = (int)hard.size() * 1000;
        S->lastDetail = Bn.detail; S->lastDetailOn = Bn.detailOn;
        if (Bn.L.ok) {
            execute(Bn.L);
            // the reserved rectangle for the Liquidity Profile (its column header goes under it): the banner's 2-line height, so the
            // header never jumps when the warning comes and goes
            bb::Rect rr; rr.l = Bn.box.l; rr.t = Bn.box.t; rr.r = Bn.box.r; rr.b = Bn.box.t + tr::bannerH(true);
            bb::publishBanner(S->symbol, S->spb, S->chartKey, nowMs, rr);
        }
        else if (region.r - region.l > 160 && region.b - region.t > 20) { FONT f; f.id = HELVETICA; f.size = 8; f.style = PLAIN; setFont(f); setTextColor(tr::C_QUIET);
            RCT rc; rc.set(coord(region.l + 4), coord(region.b - 18), coord(region.l + 4 + textW(Bn.L.note, 8, false) + 4), coord(region.b - 4)); rc.drawText(Bn.L.note.c_str(), false, false); }
    } catch (...) { S->state = "draw error"; }
    writeStatus(*S);
    return RTX_OK;
}


// (1.3.0, 2026-10-18, Rassul 2026-10-17 23:18 "the indicators must ... be aware of each other ... so the liquidity reader can better judge")
// Read every indicator's facts (MarketState.h), step the story of the zone price is working once per CLOSED bar (ZoneStory.h), look up
// its odds (question ZONE_HOLD, keys step + path, from the nightly market learning; '?' until it exists / is LIVE), post SRC READ and
// write lsFlexLevels\MarketState\<MKT>-<spb>.json every 2 s for Claude's live tools. Nothing new is drawn (mockup first).
void TapeReader::marketState(TRState& S, int kClosed, long long nowMs)
{
    if (S.chartKey.empty()) { const char* cl = getChartLabel(); S.chartKey = cl ? std::string(cl) : std::string(); }
    S.msF = mst::readAll(S.symbol, S.spb, S.chartKey, nowMs);
    if (kClosed < 0 || kClosed >= (int)S.bars.size()) return;
    const tr::Bar& kb = S.bars[(size_t)kClosed];
    if (kb.raw != S.storyRaw) {                                     // once per closed bar
        S.storyRaw = kb.raw;
        auto L = S.msF.find("LIQ");
        if (L != S.msF.end()) {
            zst::Zone z; z.id = mst::getS(L->second, "zone_id"); z.lo = mst::getD(L->second, "zone_lo"); z.hi = mst::getD(L->second, "zone_hi");
            const std::string sd = mst::getS(L->second, "zone_side"); z.side = sd == "support" ? 1 : sd == "resistance" ? -1 : 0;
            if (!z.id.empty() && z.side != 0 && z.hi >= z.lo && (z.id != S.story.z.id || S.story.done)) { S.story = zst::Story(); S.story.z = z; }
        }
        if (S.story.z.side != 0) {
            zst::Bar b; b.t = kb.t; b.o = kb.o; b.h = kb.h; b.l = kb.l; b.c = kb.c;
            zst::step(S.story, b, zst::observe(S.story.z, b, S.msF));
        }
        // the odds of this story state (from the nightly market learning)
        if (S.oddsChecked < 0 || nowMs - S.oddsChecked >= 60000 || nowMs < S.oddsChecked) {
            S.oddsChecked = nowMs; std::string text;
            std::ifstream in((flexDir() + "LRA-Odds-" + S.mkt + ".csv").c_str(), std::ios::binary);
            if (in) { in.seekg(0, std::ios::end); const std::streamoff sz = in.tellg(); if (sz > 0 && sz <= (std::streamoff)odt::MAX_BYTES) { in.seekg(0); text.resize((size_t)sz); in.read(&text[0], sz); } }
            if (!text.empty()) { odt::Table t = odt::parse(text, S.mkt); if (t.ok) { S.odds = t; S.oddsWhy = "ok, built " + t.built; } else S.oddsWhy = t.why; } else S.oddsWhy = "missing";
        }
        const std::string path = zst::pathSoFar(S.story);
        const odt::Answer A = odt::lookup(S.odds, "ZONE_HOLD", { zst::stepName(S.story.step), path });
        S.readWords = zst::words(S.story); S.readOdds = (A.ok && A.live) ? std::to_string(odt::pct(A.p)) + "%" : "?";
        std::map<std::string, std::string> kv;
        kv["zone_id"] = S.story.z.id.empty() ? "?" : S.story.z.id; kv["story"] = zst::stepName(S.story.step); kv["story_path"] = path;
        kv["odds"] = (A.ok && A.live) ? mst::num(A.p, 3) : "?"; kv["odds_q"] = "ZONE_HOLD"; kv["odds_base"] = A.ok ? mst::num(A.base, 3) : "?"; kv["odds_n"] = std::to_string(A.n);
        const bool abs = S.story.step >= zst::ABSORBING && S.story.step != zst::BREAK && S.story.step != zst::DEF_PULLING;
        kv["badge"] = abs ? "ABS?" : "";               // Rassul 2026-10-18: ABS first, with "?" until the market's knowledge grade is Solid
        kv["grade"] = "?";                             // the knowledge grade arrives with the nightly market learning
        kv["words"] = S.readWords;
        mst::publish("READ", S.symbol, S.spb, S.chartKey, nowMs, kb.t, kv);
        S.msF["READ"] = mst::read("READ", S.symbol, S.spb, S.chartKey, nowMs);
    }
    if (S.snapMs < 0 || nowMs - S.snapMs >= 2000 || nowMs < S.snapMs) {
        S.snapMs = nowMs;
        const std::string F = flexDir(); if (F.empty()) return;
        makeDir(F + "MarketState");
        const std::string ct = std::to_string(S.bars.empty() ? 0 : S.bars.back().t);
        if (writeAtomic(F + "MarketState\\" + S.mkt + "-" + std::to_string(S.spb) + ".json", mst::snapshotJson(S.mkt, S.symbol, S.spb, ct, nowMs, S.msF))) ++S.snapWrites;
    }
}

void TapeReader::writeStatus(TRState& S)
{
    if (S.mkt.empty()) return;
    std::string o = "VERSION," TR_VERSION "\nMARKET," + S.mkt + "\nSYMBOL," + S.symbol + "\nSPB," + std::to_string(S.spb) + "\nSTATE," + S.state + "\nPOSITION," + std::to_string(S.pos) + "\n";
    auto in = [&](const char* name, const Src& s) { o += std::string("INPUT,") + name + "," + (s.good ? "found" : s.res == tr::RD_MISSING ? "missing" : "not used") + "," + s.note + "," + s.path + "\n"; };
    in("DeltaProfile", S.sDp); for (int k = 0; k < tr::NSRC; ++k) in(tr::SRC_NAME[k], S.sOf[k]);
    in("LiqZones", S.sLiqZ); in("LiquidityStatus", S.sLiqSt); in("KeyLevels", S.sKey); in("Levels", S.sMq); in("Touch", S.sTouch);
    in("Dealer", S.sDealer); in("HodLod", S.sHl); in("Turns", S.sTurns); in("Params", S.sParams);
    in("Options", S.sOpt); in("OrderFlowGates", S.sOfParams);   // (1.1.0)
    o += "OPTIONS," + S.optWhy + ",grids " + std::to_string(S.optHist.v.size()) + ",gates";
    for (int g = 0; g < ofo::NGATE; ++g) o += std::string(" ") + ofo::GATE_KEY[g] + "=" + ofo::gateText(S.gates[g]);
    o += "\n";
    o += "PARAMS," + S.P.src + (S.P.why.empty() ? "" : " (" + S.P.why + ")") + ",weights " + std::to_string(S.P.w[0]).substr(0, 4) + "/" + std::to_string(S.P.w[1]).substr(0, 4) + "/" + std::to_string(S.P.w[2]).substr(0, 4) + "/" + std::to_string(S.P.w[3]).substr(0, 4) + "\n";
    o += "TURNS," + S.turns.why + "\n";
    if (S.haveLive) {
        const tr::Read& R = S.live;
        o += "LAST_READ," + R.ct + "," + R.biasText + ",flow " + std::to_string(R.votes[0]) + " (" + R.why[0] + "),level " + std::to_string(R.votes[1]) + " (" + R.why[1] + "),gamma " + std::to_string(R.votes[2]) + " (" + R.why[2] + "),trend " + std::to_string(R.votes[3]) + " (" + R.why[3] + ")" + (R.intrabar ? ",intrabar" : "") + "\n";
        if (!R.turnRow.empty()) o += "TURN_ROW," + R.turnRow + "\n";
        o += "LINE1," + R.turn + "\nBUYERS," + R.buyers + "\nSELLERS," + R.sellers + "\nOPTIONS_1," + R.opt[0] + "\nOPTIONS_2," + R.opt[1] + "\nLEVELS (logged, not drawn)," + R.levels + "\nINPUTS_USED," + R.inputs + "\n";
    }
    o += "LOG," + S.logPath + ",writes " + std::to_string(S.logWrites) + ",fails " + std::to_string(S.logFails) + ",queued " + std::to_string(S.logQueue.size()) + ",loaded " + std::to_string(S.logLoaded) + ",rejected " + std::to_string(S.logRejected) + "\n";
    o += "LAYOUT," + std::string(S.lastOk ? "drawn" : "not drawn") + "," + S.layoutNote + ",box " + std::to_string(S.lastBox.l) + " " + std::to_string(S.lastBox.t) + " " + std::to_string(S.lastBox.r) + " " + std::to_string(S.lastBox.b) +
         ",region " + std::to_string(S.lastRegion.l) + " " + std::to_string(S.lastRegion.r) + ",lines " + std::to_string(S.lastLines) + ",spot " + std::to_string(S.lastSpot) + ",avoid tables " + std::to_string(S.lastAvoid / 1000) + " candles " + std::to_string(S.lastAvoid % 1000) + "\n";
    o += "BANNER,pos " + std::to_string(S.place.pos) + ",dx " + std::to_string(S.place.dx) + ",dy " + std::to_string(S.place.dy) + ",background " + (S.bg ? "on" : "off") + ",width " + std::to_string(S.banW.w)
         + ",warning " + (S.warnSeen ? (S.warn.on ? S.warn.name : std::string("none")) : std::string("no Liquidity Profile on this chart")) + ",hover " + (S.inBanner ? "on" : "off") + ",hovers " + std::to_string(S.bannerHovers) + ",moves " + std::to_string(S.hyst.moves)
         + (S.placeWhy.empty() ? "" : "," + S.placeWhy) + "\n";
    o += "MARKET_STATE,sources " + std::to_string(S.msF.size()) + ",story " + zst::stepName(S.story.step) + " " + zst::pathSoFar(S.story) + " " + S.story.z.id + ",odds " + S.readOdds + " (" + S.oddsWhy + "),snapshots " + std::to_string(S.snapWrites) + "\n";
    o += "COUNTS,recomputes " + std::to_string(S.recomputes) + ",redraws " + std::to_string(S.redraws) + ",hover " + std::to_string(S.hover) + "\n";
    const long long now = steadyMs();
    const bool changed = o != S.lastStatus;
    if (!(changed && (S.lastStatusMs < 0 || now - S.lastStatusMs >= 2000)) && S.lastStatusMs >= 0 && now - S.lastStatusMs < 30000) return;
    const std::string F = flexDir(); if (F.empty()) return;
    if (writeAtomic(F + "TapeReader.status-" + S.mkt + ".txt", o)) { S.lastStatus = o; S.lastStatusMs = now; ++S.statusWrites; }
}

extern "C" cppExtension *CreateExtension(void)
{
    TapeReader *p = new TapeReader();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setDescription("LRA Tape Reader: one box with the combined read of the other indicators - the bias (FLOW, LEVEL, GAMMA, TREND votes), the turn / swing odds at the level in play and the next level; what BUYERS and SELLERS did, in lots with the ratio and the pace, levels named inside (green = bullish, red = bearish, grey = quiet); and OPTIONS (blue): who traded options and what dealers must do in futures, from the latest MenthorQ snapshot that has arrived (hidden when older than 20 min, '?' until proven in this market). Mouse over a candle shows the read logged for it. A 1-2 line banner at the top left (the read + the Liquidity warning); mouse over it for BUYERS / SELLERS / OPTIONS. Settings: Position, Offset X / Y, Banner background.");
    p->setVersion("1.3.4");   // TR_VERSION = tr::VERSION = trset::PLUGIN_VERSION
    return p;
}
