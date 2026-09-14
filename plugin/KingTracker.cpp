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
 *
 *  A SEPARATE indicator from lsDayModel / lsDayStats / lsGammaProfile, so its
 *  dialog and parameter numbering never destabilise the others.
 *
 *  Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv (shared; each
 *  plugin ignores the other's rows):
 *     KINGTRACK,<fam ES|NQ>,<book SPX|SPY|QQQ|NDX>,<secOfDay>,<futPrice>,<strike>
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
static const COLOR C_SPX = 0x00E040C0;  // SPX King  magenta (matches gamma King)
static const COLOR C_SPY = 0x0040C0E0;  // SPY King  cyan
static const COLOR C_QQQ = 0x0033B36B;  // QQQ King  green
static const COLOR C_NDX = 0x00E0A030;  // NDX King  amber

// darker outline for the filled roll dots
static COLOR darker(COLOR a) {
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    ar=(int)(ar*0.55f); ag=(int)(ag*0.55f); ab=(int)(ab*0.55f);
    return (COLOR)(((COLOR)ar<<16) | ((COLOR)ag<<8) | (COLOR)ab);
}

// the four books; index 0..3
enum { B_SPX = 0, B_SPY, B_QQQ, B_NDX, NBOOK };
static const char* BOOK_NAME[NBOOK] = { "SPX", "SPY", "QQQ", "NDX" };
static const char* BOOK_FAM [NBOOK] = { "ES",  "ES",  "NQ",  "NQ"  };
static const COLOR BOOK_DEF [NBOOK] = { C_SPX, C_SPY, C_QQQ, C_NDX };

struct Step { double so; float px; int strike; };
struct BookTrk { std::vector<Step> steps; bool hasNow; float nowPx; int nowStrike, nowPct; };

// ---- parameter indices ----------------------------------------------------
struct PIdx {
    int family, width, offset, labels, dots, font;
    int show[NBOOK], col[NBOOK];
};
static PIdx PX;

struct Settings {
    int family;          // 0 Auto, 1 ES, 2 NQ
    int width, offset, font;
    bool labels, dots;
    bool show[NBOOK];
    COLOR col[NBOOK];
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

    void load();
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
    return RTX_OK;
}

void KingTracker::readSettings(Settings& S)
{
    S.family = getListIndex(PX.family);
    S.width  = getIntegerValue(PX.width);   if (S.width < 1) S.width = 1; if (S.width > 8) S.width = 8;
    for (int i = 0; i < NBOOK; i++) {
        S.show[i] = isBoxChecked(PX.show[i]) != 0;
        COLOR c = (COLOR)(getIntegerValue(PX.col[i]) & 0xFFFFFF); S.col[i] = c ? c : BOOK_DEF[i];
    }
    S.labels = isBoxChecked(PX.labels) != 0;
    S.dots   = isBoxChecked(PX.dots)   != 0;
    S.offset = getIntegerValue(PX.offset);  if (S.offset < -720) S.offset = -720; if (S.offset > 720) S.offset = 720;
    S.font   = getIntegerValue(PX.font);    if (S.font < 7) S.font = 7; if (S.font > 40) S.font = 40;
}

// ---- data load ------------------------------------------------------------
void KingTracker::load()
{
    for (int i = 0; i < NBOOK; i++) { book[i].steps.clear(); book[i].hasNow = false; }
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
            for (int i = 0; i < NBOOK; i++) if (t[2] == BOOK_NAME[i]) { bi = i; break; }
            if (bi < 0) continue;
            Step s; s.so = atof(t[3].c_str()); s.px = (float)atof(t[4].c_str()); s.strike = atoi(t[5].c_str());
            book[bi].steps.push_back(s);
        } else if (t[0] == "KINGNOW" && t.size() >= 5) {
            for (int i = 0; i < NBOOK; i++) if (t[2] == BOOK_NAME[i]) { bi = i; break; }
            if (bi < 0) continue;
            book[bi].nowPx = (float)atof(t[3].c_str());
            book[bi].nowStrike = atoi(t[4].c_str());
            book[bi].nowPct = (t.size() > 5) ? atoi(t[5].c_str()) : 0;
            book[bi].hasNow = true;
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
        if (strcmp(BOOK_FAM[b], famStr) != 0) continue;   // only this chart's family
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
int KingTracker::draw(void) { load(); render(cfg); return RTX_OK; }

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    KingTracker *p = new KingTracker();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("King tracker stepped lines (SPX/SPY on ES, QQQ/NDX on NQ), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.1");
    return p;
}
