/********************************************************************************
 *  KingTracker.cpp  —  Investor/RT RTX extension  (KING TRACKER — stepped lines)
 *
 *  Draws each book's King as a TIME-indexed STEPPED LINE across the session: the
 *  King holds a flat level, then steps to a new price when it rolls to a new
 *  strike. Price-aligned (INSTRUMENT_SCALE) so each step sits at the King's own
 *  futures price; time-positioned by mapping the roll clock to a chart bar.
 *
 *    ES chart : SPX King (magenta) · SPY King (cyan)
 *    NQ chart : QQQ King (green)   · NDX King (amber)
 *    (v0.11) Source = IF : the IF Magnet journey instead (book IF on ES, IFQ on NQ; panel 16.38) — InsiderFinance's
 *    0DTE King, one source OR the other (operator: "switch back and forth between IF and Skylit"), IF colour.
 *
 *  A SEPARATE indicator from lsDayModel / lsDayStats / lsGammaProfile, so its
 *  dialog and parameter numbering never destabilise the others.
 *
 *  Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv (shared; each
 *  plugin ignores the other's rows):
 *     KINGTRACK,<fam ES|NQ>,<book SPX|SPY|QQQ|NDX|IF|IFQ>,<secOfDay>,<futPrice>,<strike>
 *          one confirmed step (a King-strike change) at its futures price/time
 *     KINGNOW,<fam>,<book>,<futPrice>,<strike>,<pct>
 *          the live right-edge level (kept aligned with the gamma magenta King)
 *
 *  The chart's family (ES/NQ) is auto-detected from the root symbol; a setting
 *  overrides it. A Clock-offset (min) setting calibrates the roll clock (the CSV
 *  is CT) to the chart's own timezone — tune it once against the live chart.
 *
 *  Parameter indices numbered EXPLICITLY (pc++); NO setLabelParameter headers.
 ********************************************************************************/
#include "irtsdk.h"
#include "ContractOffsetLogic.h"   // (shared) the anchor-bar rule, tested
#include "KingTrackerLogic.h"      // (v0.10) the row grammar, the re-derivation, the anchor — testable without IRT
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

// ---- palette --------------------------------------------------------------
static const COLOR C_SPX = 0x00F0D024;  // SPX King  yellow  (v0.4 default matches the operator's picker selection)
static const COLOR C_SPY = 0x00F5883A;  // SPY King  orange  (v0.4 default matches the operator's picker selection)
static const COLOR C_QQQ = 0x0033B36B;  // QQQ King  green
static const COLOR C_NDX = 0x00E0A030;  // NDX King  amber
static const COLOR C_IF  = 0x00FF60D0;  // (v0.11) IF Magnet  magenta-pink — the IF rail's own colour family, so the eye knows which source is on

// darker outline for the filled roll dots
static COLOR darker(COLOR a) {
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    ar=(int)(ar*0.55f); ag=(int)(ag*0.55f); ab=(int)(ab*0.55f);
    return (COLOR)(((COLOR)ar<<16) | ((COLOR)ag<<8) | (COLOR)ab);
}

// the six books; index 0..5 — (v0.11) IF = InsiderFinance's SPX 0DTE Magnet journey (panel 16.38), IFQ = its QQQ one
enum { B_SPX = 0, B_SPY, B_QQQ, B_NDX, B_IF, B_IFQ, NBOOK };
static const char* BOOK_NAME[NBOOK] = { "SPX", "SPY", "QQQ", "NDX", "IF",  "IFQ" };
static const char* BOOK_FAM [NBOOK] = { "ES",  "ES",  "NQ",  "NQ",  "ES",  "NQ"  };
static const COLOR BOOK_DEF [NBOOK] = { C_SPX, C_SPY, C_QQQ, C_NDX, C_IF,  C_IF  };

typedef ktl::Step Step;   // (v0.10) KingTrackerLogic.h
typedef ktl::Book BookTrk;

// ---- parameter indices ----------------------------------------------------
struct PIdx {
    int family, width, offset, labels, dots, font;
    int show[NBOOK], col[NBOOK];
    int source;          // (v0.11) Skylit | IF
};
static PIdx PX;

struct Settings {
    int family;          // 0 Auto, 1 ES, 2 NQ
    int width, offset, font;
    bool labels, dots;
    bool show[NBOOK];
    COLOR col[NBOOK];
    bool sourceIF;       // (v0.11) false = Skylit's tape King (SPX/SPY · QQQ/NDX), true = IF's Magnet (IF · IFQ)
};

// ---------------------------------------------------------------------------
class KingTracker : public cppExtension {
public:
    KingTracker();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    BookTrk book[NBOOK];
    Settings cfg;
    int lastBar;
    double asofSo;              // (v0.2) ASOF write-time (CT sec-of-day) for the STALE badge; <0 = unknown
    float spotPx; bool hasSpot; // (v0.3) SPOT anchor for the contract offset
    float scaleRef; bool hasScaleRef;   // (v0.5) front-month ES anchor (SCALEREF row) — the King prices' scale
    double scaleRefSo; int scaleRefY, scaleRefM, scaleRefD;   // (v0.9) the CT minute + date of that quote (panel 16.34); so<0 = unknown

    void load();
    void drawStaleBadge();     // (v0.2) red badge if the CSV has gone cold
    void applyContractOffset();// (v0.3) shift the King lines onto this chart's own contract price
    void readSettings(Settings& S);
    void render(const Settings& S);
    int  chartFamily(const Settings& S);        // returns 1 ES / 2 NQ
    short yOf(float price);
    short xOfBar(int bar);
    int  barForClock(double secOfDay, int offMin);
    void seg(short x1, short y1, short x2, short y2, COLOR col, int w);
    void dot(short x, short y, COLOR col, int r);
    void label(short x, short y, const char* s, COLOR col, int sz);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- constructor: safe defaults -------------------------------------------
KingTracker::KingTracker() : cppExtension()
{
    lastBar = 0;
    cfg.family = 0;      // Auto
    cfg.width  = 2;
    cfg.offset = 0;      // clock offset minutes (tz/basis calibration)
    cfg.labels = true;
    cfg.dots   = true;
    cfg.font   = 10;
    cfg.sourceIF = false;   // (v0.11) Skylit until he switches
    for (int i = 0; i < NBOOK; i++) { cfg.show[i] = true; cfg.col[i] = BOOK_DEF[i]; book[i].hasNow = false; }
}

// ---- parameter callbacks (guard on a plausible font read) -----------------
int KingTracker::parmsLoad(void)  { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }
int KingTracker::parmsApply(void) { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }
int KingTracker::parmsUpdt(unsigned int) { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }

// ---- parameter panel ------------------------------------------------------
int cppExtension::setup(void)
{
    setParameterVersion(1);            // v0.1 first release — establishes the defaults below
    setParameterDialogHeight(20);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.family = pc++; setListParameter   ("Chart family", 0, "Auto;ES;NQ");
    PX.width  = pc++; setIntegerParameter("Line width px", 2, 0, SL);
    PX.show[B_SPX] = pc++; setBoolParameter ("SPX King (ES)", true);
    PX.col [B_SPX] = pc++; setColorParameter("SPX colour", C_SPX, 0, SL);
    PX.show[B_SPY] = pc++; setBoolParameter ("SPY King (ES)", true);
    PX.col [B_SPY] = pc++; setColorParameter("SPY colour", C_SPY, 0, SL);
    PX.show[B_QQQ] = pc++; setBoolParameter ("QQQ King (NQ)", true);
    PX.col [B_QQQ] = pc++; setColorParameter("QQQ colour", C_QQQ, 0, SL);
    PX.show[B_NDX] = pc++; setBoolParameter ("NDX King (NQ)", true);
    PX.col [B_NDX] = pc++; setColorParameter("NDX colour", C_NDX, 0, SL);
    PX.labels = pc++; setBoolParameter   ("Right-edge labels", true);
    PX.dots   = pc++; setBoolParameter   ("Roll-point dots", true, SL);
    PX.offset = pc++; setIntegerParameter("Clock offset (min)", 0, 0);
    PX.font   = pc++; setIntegerParameter("Font size (pt)", 10, 0, SL);
    // (v0.11) appended AFTER the v0.1 parameters so saved instances keep their indices; the IF books have no Show box —
    // the Source switch is their visibility (one source or the other, never both)
    PX.source     = pc++; setListParameter ("Source", 0, "Skylit;IF");
    PX.col[B_IF]  = pc++; setColorParameter("IF colour", C_IF, 0, SL);
    PX.col[B_IFQ] = PX.col[B_IF];
    PX.show[B_IF] = PX.show[B_IFQ] = -1;
    return RTX_OK;
}

void KingTracker::readSettings(Settings& S)
{
    S.family = getListIndex(PX.family);
    S.width  = getIntegerValue(PX.width);   if (S.width < 1) S.width = 1; if (S.width > 8) S.width = 8;
    for (int i = 0; i < NBOOK; i++) {
        S.show[i] = (PX.show[i] < 0) ? true : (isBoxChecked(PX.show[i]) != 0);   // (v0.11) the IF books have no box
        COLOR c = (COLOR)(getIntegerValue(PX.col[i]) & 0xFFFFFF); S.col[i] = c ? c : BOOK_DEF[i];
    }
    S.sourceIF = (getListIndex(PX.source) == 1);   // (v0.11)
    S.labels = isBoxChecked(PX.labels) != 0;
    S.dots   = isBoxChecked(PX.dots)   != 0;
    S.offset = getIntegerValue(PX.offset);  if (S.offset < -720) S.offset = -720; if (S.offset > 720) S.offset = 720;
    S.font   = getIntegerValue(PX.font);    if (S.font < 7) S.font = 7; if (S.font > 40) S.font = 40;
}

// ---- data load ------------------------------------------------------------
void KingTracker::load()
{
    for (int i = 0; i < NBOOK; i++) { book[i].steps.clear(); book[i].hasNow = false; }
    asofSo = -1; hasSpot = false; spotPx = 0.0f; hasScaleRef = false; scaleRef = 0.0f; scaleRefSo = -1; scaleRefY = scaleRefM = scaleRefD = 0;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";
    std::ifstream f(path.c_str()); if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t; std::stringstream ss(line); std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.size() < 2) continue;
        int bi = -1;
        if (t[0] == "KINGTRACK" && t.size() >= 6) {
            std::string fam, bk; Step s; if (!ktl::parseTrack(t, fam, bk, s)) continue;   // (v0.10) KingTrackerLogic.h
            for (int i = 0; i < NBOOK; i++) if (bk == BOOK_NAME[i]) { bi = i; break; }
            if (bi < 0) continue;
            book[bi].steps.push_back(s);
        } else if (t[0] == "KINGNOW" && t.size() >= 5) {
            std::string fam, bk; ktl::Book nb; if (!ktl::parseNow(t, fam, bk, nb)) continue;
            for (int i = 0; i < NBOOK; i++) if (bk == BOOK_NAME[i]) { bi = i; break; }
            if (bi < 0) continue;
            book[bi].nowPx = nb.nowPx; book[bi].nowStrike = nb.nowStrike; book[bi].nowPct = nb.nowPct; book[bi].hasNow = true;
        } else if (t[0] == "ASOF" && t.size() >= 2) {
            asofSo = atof(t[1].c_str());
        } else if (t[0] == "SPOT" && t.size() >= 2) {
            spotPx = (float)atof(t[1].c_str()); hasSpot = true;
        } else if (t[0] == "SCALEREF" && t.size() >= 2) {
            scaleRef = (float)atof(t[1].c_str()); hasScaleRef = true; scaleRefSo = -1; scaleRefY = scaleRefM = scaleRefD = 0;
            if (t.size() >= 4) { scaleRefSo = atof(t[2].c_str()); sscanf(t[3].c_str(), "%d-%d-%d", &scaleRefY, &scaleRefM, &scaleRefD); }   // (v0.9) the quote's own minute
        }
    }
}

// ---- family ---------------------------------------------------------------
int KingTracker::chartFamily(const Settings& S)
{
    if (S.family == 1) return 1;
    if (S.family == 2) return 2;
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);   // "ES", "NQ", "MES", "MNQ", ...
    std::string r = rs ? rs : "";
    if (r.find("NQ") != std::string::npos) return 2;
    return 1;   // default ES
}

// ---- geometry helpers -----------------------------------------------------
short KingTracker::yOf(float price) { PNT p; p.set(lastBar, price); return p.v; }
short KingTracker::xOfBar(int bar)  { PNT p; p.set(bar, 0.0f); return p.h; }

int KingTracker::barForClock(double secOfDay, int offMin)
{
    // build "today at secOfDay (+offset)" and map to the first bar >= that time
    RTDATE nowd = currentDate();
    struct tm tmv; memset(&tmv, 0, sizeof(tmv));
    getLocaltime(nowd, &tmv);
    long tot = (long)(secOfDay + 0.5) + (long)offMin * 60;
    if (tot < 0) tot = 0; if (tot > 86399) tot = 86399;
    tmv.tm_hour = (int)(tot / 3600);
    tmv.tm_min  = (int)((tot % 3600) / 60);
    tmv.tm_sec  = (int)(tot % 60);
    RTDATE rd = makeRTDATE(&tmv);
    RTARRAYI dates(barDateTime);
    int bar = -1;
    RTX_RESULT rr = dates.getBarNumber(rd, &bar, 0, kDATE_GE);   // first bar at/after the roll time
    if (rr != RTX_OK || bar < 0 || bar > lastBar) {
        // fallback: scan the bar-datetime array directly (also covers a future time -> last bar)
        bar = -1;
        long cnt = dates.count;
        if (cnt > 0) {
            for (long i = 0; i < cnt; i++) { if ((RTDATE)dates[i] >= rd) { bar = (int)i; break; } }
        }
        if (bar < 0) bar = lastBar;     // time is after every bar (i.e. "now") — clamp to the right edge
    }
    if (bar > lastBar) bar = lastBar;
    if (bar < 0) bar = 0;
    return bar;
}

void KingTracker::seg(short x1, short y1, short x2, short y2, COLOR col, int w)
{
    setPen(col, (short)w, P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
}
void KingTracker::dot(short x, short y, COLOR col, int r)
{
    RCT rc; rc.set((short)(x - r), (short)(y - r), (short)(x + r), (short)(y + r));
    rc.draw(1, darker(col), col, DRAW_OPAQUE, PAT_SOLID);
}
void KingTracker::label(short x, short y, const char* s, COLOR col, int sz)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = BOLD; setFont(f);
    setTextColor(col);
    RCT rc; rc.set(x, (short)(y - sz), (short)(x + 140), (short)(y + sz));
    rc.drawText(s, false, false);
}

// ---- render ---------------------------------------------------------------
void KingTracker::render(const Settings& S)
{
    long n = getBarCount(); if (n < 2) return;
    lastBar = (int)n - 1;
    int fam = chartFamily(S);        // 1 ES, 2 NQ
    const char* famStr = (fam == 2) ? "NQ" : "ES";
    short xRight = xOfBar(lastBar);

    for (int b = 0; b < NBOOK; b++) {
        if (!S.show[b]) continue;
        if (!ktl::bookDrawn(BOOK_NAME[b], BOOK_FAM[b], famStr, S.sourceIF)) continue;   // (v0.11) this chart's family AND the chosen source
        BookTrk& T = book[b];
        if (T.steps.empty() && !T.hasNow) continue;
        COLOR col = S.col[b];

        // build (x,y) points: each confirmed step, then a synthetic 'now' point at
        // the right edge (KINGNOW price if present, else the last step's price).
        std::vector<short> xs, ys;
        for (size_t i = 0; i < T.steps.size(); i++) {
            int bar = barForClock(T.steps[i].so, S.offset);
            xs.push_back(xOfBar(bar)); ys.push_back(yOf(T.steps[i].px));
        }
        float nowPx = T.hasNow ? T.nowPx : (T.steps.empty() ? 0.0f : T.steps.back().px);
        xs.push_back(xRight); ys.push_back(yOf(nowPx));

        // stepped line: horizontal at y_i to x_{i+1}, then vertical to y_{i+1}
        for (size_t i = 0; i + 1 < xs.size(); i++) {
            seg(xs[i], ys[i], xs[i+1], ys[i], col, S.width);        // hold flat
            seg(xs[i+1], ys[i], xs[i+1], ys[i+1], col, S.width);    // step
        }
        // roll-point dots (skip the synthetic 'now' point)
        if (S.dots) {
            int r = S.width + 1; if (r < 2) r = 2;
            for (size_t i = 0; i < T.steps.size() && i < xs.size(); i++) dot(xs[i], ys[i], col, r);
        }
        // right-edge label: "SPX 7700"
        if (S.labels && !xs.empty()) {
            char lab[24];
            int strike = T.hasNow ? T.nowStrike : (T.steps.empty() ? 0 : T.steps.back().strike);
            sprintf_s(lab, sizeof(lab), "%s %d", BOOK_NAME[b], strike);
            label((short)(xRight + 4), ys.back(), lab, col, S.font);
        }
    }
}

// ---- draw() ---------------------------------------------------------------
int KingTracker::draw(void) { load(); applyContractOffset(); render(cfg); drawStaleBadge(); return RTX_OK; }

// ---- (v0.3) CONTRACT ALIGNMENT — the ES King prices are in the panel's ES space (SPOT is the ES-cash
// anchor). If the ES chart is a different contract (EPZ26 December, ~+70 over cash), shift every King
// price by (this chart's last close − SPOT) so the lines sit on the chart. On an NQ chart the books are
// ~29k, so chartClose − SPOT is huge and hits the clamp → no shift (the NQ books already draw in NQ1
// space; a dedicated NQ spot anchor is a later refinement).
void KingTracker::applyContractOffset()
{
    long n = getBarCount(); if (n < 1) return;
    RTARRAY close(barClose);
    // (GP 0.46 / KT 0.7) ANCHOR ON THE BAR THE CSV WAS WRITTEN AT, NOT THE LIVE CLOSE. SCALEREF is the ES1 price at the
    // panel's export (ASOF); comparing it to the LIVE last close made off = (price now - price 0..3 min ago) + basis, so
    // the whole profile slid up and down with every tick between exports — operator, 2026-09-16 13:20: "the entire
    // profile is moving up and down". The basis is a property of the two contracts, not of the last tick: take the
    // chart's close at the ASOF second (the last bar stamped at or before it on the last bar's date). Falls back to
    // the live close when ASOF is absent or no bar matches (pre-open, a CSV from another day).
    float chartClose = close[(int)n - 1];
    {
        // (GP 0.56 / KT 0.10) THE ANCHOR RULE LIVES IN plugin/ContractOffsetLogic.h (col::anchorIndex) — shared by both
        // plugins and pinned by plugin/test_contractoffset_logic.cpp, after it failed twice on 2026-09-16 while it lived
        // behind the SDK in each .cpp. The bars are handed over as plain (date, sec-of-day, close) records.
        RTARRAYI dt(barDateTime);
        int from = (int)n - 6000; if (from < 0) from = 0;
        std::vector<col::Bar> bars; bars.reserve((size_t)((int)n - from));
        for (int i = from; i < (int)n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
            col::Bar b; b.y = t.tm_year + 1900; b.m = t.tm_mon + 1; b.d = t.tm_mday;
            b.sod = t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec; b.close = close[i];
            bars.push_back(b);
        }
        int ai = col::anchorIndex(bars.empty() ? 0 : &bars[0], (int)bars.size(), asofSo, scaleRefSo, scaleRefY, scaleRefM, scaleRefD);
        if (ai >= 0) chartClose = bars[(size_t)ai].close;
    }
    if (!(chartClose > 0)) return;
    // (v0.5) ANCHOR ON SCALEREF, NOT SPOT. The ES King prices (KINGNOW/KINGTRACK futPrice) are in the
    // panel's front-month ES scale (esOfSpx); SPOT is the day model's own value, ~this chart's (Dec) scale
    // during the roll, so anchoring on it made off≈0 and the front-scale King lines stayed a full calendar
    // spread below price. SCALEREF is the front price those King prices are in, so off = chartClose −
    // SCALEREF = the Sep→Dec spread → the lines land on the charted contract. Falls back to SPOT if absent.
    // The ±300 clamp still protects the NQ books on an NQ chart (chartClose~29k − SCALEREF~7.6k is huge).
    // (v0.6) HISTORY IN THE CURRENT SCALE. Each KINGTRACK step stores the ES price AT THE TIME OF THE ROLL.
    // When Skylit's ES1 rolls to the next contract mid-session (it did on 2026-09-16 between 09:47 and
    // 10:38 — 7688 became 7757 for the same SPX 7685 King), the earlier steps are still in the OLD scale
    // while KINGNOW and SCALEREF are in the NEW one, so the pre-roll line drew a full calendar spread too
    // low with a phantom step at the roll. The strike is scale-free, so re-derive every step's price as
    // strike x (nowPx / nowStrike) — the ratio the CURRENT price is in. Basis drift within a day is a
    // point or two; the roll is ~70. Only when a KINGNOW row gives the ratio; otherwise the stored prices.
    for (int b = 0; b < NBOOK; b++) ktl::rederive(book[b]);                     // (v0.10) KingTrackerLogic.h
    float anchor = 0.0f, off = 0.0f;
    if (!ktl::anchorPrice(hasScaleRef, scaleRef, hasSpot, spotPx, anchor)) return;
    if (!ktl::offsetFor(chartClose, anchor, off)) return;
    for (int b = 0; b < NBOOK; b++) ktl::shift(book[b], off);
}

// ---- (v0.2) STALE badge — ASOF,<CT sec-of-day> vs the chart clock; > ~4 min ⇒ frozen file (overnight-safe)
void KingTracker::drawStaleBadge()
{
    if (asofSo < 0) return;
    RTDATE now = currentDate(); struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime(now, &tmv);
    double localSo = tmv.tm_hour * 3600.0 + tmv.tm_min * 60.0 + tmv.tm_sec;
    double ageMin = ktl::staleAge(asofSo, localSo);   // (v0.10) KingTrackerLogic.h
    if (ageMin <= 4.0) return;
    char b[40];
    if (ageMin >= 90.0) sprintf_s(b, sizeof(b), "STALE %dh", (int)(ageMin / 60.0 + 0.5));
    else                sprintf_s(b, sizeof(b), "STALE %dm", (int)(ageMin + 0.5));
    RCT pane; pane.getPaneRect(false);
    FONT f; f.id = HELVETICA; f.size = 11; f.style = BOLD; setFont(f);
    short tw = (short)getTextWidth(b, -1);
    short x = (short)(pane.left + 6), y = (short)(pane.top + 6);
    RCT bg; bg.set(x, y, (short)(x + tw + 14), (short)(y + 18));
    bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
    setTextColor(0x00FF9A8F);
    RCT tr; tr.set((short)(x + 7), (short)(y + 1), (short)(x + tw + 14), (short)(y + 17));
    tr.drawText(b, false, false);
}

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    KingTracker *p = new KingTracker();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("King tracker stepped lines (SPX/SPY on ES, QQQ/NDX on NQ), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.11");
    return p;
}
