/********************************************************************************
 *  DayStats.cpp  —  Investor/RT RTX extension  (day model — the STATS STRIP)
 *
 *  Renders the §10.2 day-model statistics strip: two rows, ACTUAL (A) over
 *  EXPECTED (E, the base-rate model), so every live number reads against its own
 *  base rate on the line beneath it. A SEPARATE indicator from lsDayModel (the
 *  candle) and lsGammaProfile, so its dialog and parameter numbering can never
 *  destabilise the others. It draws a text panel in a corner of the pane; it does
 *  NOT price-align (no INSTRUMENT_SCALE) — it is a reference strip.
 *
 *  Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv (shared; each
 *  plugin ignores the other's rows). The two rows it consumes, ONE fixed 17-field
 *  order (raw secOfDay for clocks, raw minutes for durations, prices 2dp, empty =
 *  unknown):
 *     DAYSA,first,firstPx,firstClkSo,tookMin,bopMin,wickMin,wendSo,wickPct,mudMin,
 *           second,secondPx,secondClkSo,gapMin,rngPts,rngUsd,rngP25,rngP75
 *     DAYSE,<same 17 fields, expected/base-rate>
 *     WEEKDAY,<Mon..Fri>,<date>     header
 *  Columns rendered, A over E:
 *     1ST · [1st] · Took · BOP · Wick · W.End · Wick% · MUD · MUDt(=HL Gap−BOP) ·
 *     2ND · [2nd] · HL Gap · HL Rng
 *
 *  Parameter indices are numbered EXPLICITLY (pc++), one per control, NO
 *  setLabelParameter headers (a label row shifts IRT's parameter numbering out
 *  from under the getters and silently scrambles settings — see the gamma plugin).
 ********************************************************************************/
#include "irtsdk.h"
#include "DayStatsLogic.h"
#include "HostSlot.h"   // (v0.8) the row grammar, the formatting and the cells, testable without IRT
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

// ---- palette --------------------------------------------------------------
static const COLOR C_TXT   = 0x00DFE7F0;  // actual (A) ink            near-white
static const COLOR C_EXP   = 0x009AB0C4;  // expected (E) ink          dim slate
static const COLOR C_HEAD  = 0x00E0A030;  // column headers            amber
static const COLOR C_TITLE = 0x00FFFFFF;  // panel title               white
static const COLOR C_PANEL = 0x00141A22;  // background panel fill     dark slate
static const COLOR C_BORDER= 0x00394654;  // panel border              slate grey
static const COLOR C_UPG   = 0x002EC27E;  // (v0.4) HOD / markup     green
static const COLOR C_DNR   = 0x00F0616D;  // (v0.4) LOD / markdown   red

// ---- parameter indices ----------------------------------------------------
struct PIdx { enum { corner = 0, font, showA, showE, showhdr, bg, xoff, yoff }; };

struct Settings {
    int corner;             // 0 TL, 1 TR, 2 BL, 3 BR
    bool showA, showE, showhdr, bg;
    int font, xoff, yoff;
};

// one parsed stats row (raw strings; formatted at render)
typedef dsl::StatRow StatRow;   // (v0.8) DayStatsLogic.h

// ---------------------------------------------------------------------------
class DayStats : public cppExtension {
public:
    DayStats();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void);

private:
    // cppExtension instances can serve more than one chart.  Every mutable value
    // below therefore belongs to the active SDK host through getUserData(), not to
    // this shared extension object.
    struct State {
        StatRow A, E;
        std::string condBasis; int condLastHr, condLast30;
        int condP20, condP50, condP80;
        dsl::Read2 read2, read1;
        std::string weekday, daydate;
        double asofSo, eOpen;
        float spotPx, priceOff; bool hasSpot;
        RTDATE lastLoadDate;
        unsigned int sameDateCacheSkips;
        bool hasRead; std::string readFirst, readCall; double readPosr, readPct; long readN;
        Settings cfg;

        State() : condLastHr(-1), condLast30(-1), condP20(-1), condP50(-1), condP80(-1),
                  asofSo(-1), eOpen(-1), spotPx(0.0f), priceOff(0.0f), hasSpot(false),
                  lastLoadDate((RTDATE)~0UL), sameDateCacheSkips(0), hasRead(false),
                  readPosr(-1), readPct(-1), readN(0)
        {
            cfg.corner = 0; cfg.showA = true; cfg.showE = true; cfg.showhdr = true; cfg.bg = true;
            cfg.font = 11; cfg.xoff = 8; cfg.yoff = 8;
        }
    };

    State& state();
    HostSlot<State> slot_;
    void releaseState();
    void load();
    void drawStaleBadge();     // (v0.2) red badge if the CSV has gone cold
    void applyContractOffset();// (v0.3) match the [1st]/[2nd] price labels to the chart's contract
    void applyChartExtremes(); // (v0.10) the A row's prices = the chart's own session high / low
    void readSettings(Settings& S);
    void syncSettings();       // (v0.16) readSettings behind the font guard — called by the parms callbacks AND every draw
    void render(const Settings& S);
    // helpers
    void textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold);
    void textCJ(short xc, short y, const char* s, COLOR col, int sz, bool bold);  // (v0.5) centre on xc
    short textW(const char* s, int sz, bool bold);
    std::string clk(double so);                 // secOfDay -> "9:12am"
    std::string dur(double m);                   // minutes -> "1h 04m" / "42m"
    std::string px1(const std::string& raw);     // "6712.50" -> "6712" (int label)
    std::string cell(double v, int kind);        // kind: 0 dur, 1 clk, 2 pct
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- current host state ----------------------------------------------------
DayStats::State& DayStats::state()
{
    State* s = slot_.get(this, true);
    if (!s) throw std::bad_alloc();
    return *s;
}

void DayStats::releaseState()
{
    slot_.release(this);
}

// ---- constructor: state is allocated for the active host on first use -----
DayStats::DayStats() : cppExtension() {}

// ---- parameter callbacks (guard on a plausible font read) -----------------
// (v0.16) THE SAVED SETTINGS ARE READ ON EVERY DRAW, NOT ONLY IN THE DIALOG CALLBACKS. Operator, 2026-09-19: "Day Stats seems
// to go to upper left even though when I open up the options it says top center, then I have to select top center again and
// apply." IRT can call parmsLoad before a restored instance's values are populated — the font reads 0, the guard below skips
// readSettings, and `cfg` keeps the CONSTRUCTOR's defaults (Corner = Top-left) until an Apply fires parmsApply. The dialog was
// right all along; the plugin just never read it. syncSettings() runs at the top of draw() as well, behind the same guard, so
// the first draw after the values exist applies the saved Corner / font / insets. Eight reads per draw — nothing.
void DayStats::syncSettings() { State& s = state(); int p = getIntegerValue(PIdx::font); if (p >= 1 && p <= 200 /* (2026-09-17) was 6..48: at Font size 3 no dialog change ever applied (KT 0.12 lesson) */) readSettings(s.cfg); }
int DayStats::parmsLoad(void)  { syncSettings(); return RTX_OK; }
int DayStats::parmsApply(void) { syncSettings(); return RTX_OK; }
int DayStats::parmsUpdt(unsigned int) { syncSettings(); return RTX_OK; }
int DayStats::done(void) { releaseState(); return RTX_OK; }
int DayStats::destroy(void) { releaseState(); return RTX_OK; }

// ---- parameter panel ------------------------------------------------------
int cppExtension::setup(void)
{
    setParameterVersion(1);            // v0.1 first release — establishes the defaults below
    setParameterDialogHeight(12);
    const short SL = kParmAppendSameLine;
    setListParameter   ("Corner", 0, "Top-left;Top-right;Bottom-left;Bottom-right;Top-center;Bottom-center");   // PIdx::corner = 0; centre anchors are appended for saved values
    setIntegerParameter("Font size (pt)", 11, 0, SL); // PIdx::font = 1
    setBoolParameter   ("Actual row (A)", true);     // PIdx::showA = 2
    setBoolParameter   ("Expected row (E)", true, SL);// PIdx::showE = 3
    setBoolParameter   ("Column headers", true);      // PIdx::showhdr = 4
    setBoolParameter   ("Background panel", true, SL);// PIdx::bg = 5
    setIntegerParameter("X inset px", 8, 0);          // PIdx::xoff = 6
    setIntegerParameter("Y inset px", 8, 0, SL);      // PIdx::yoff = 7
    return RTX_OK;
}

void DayStats::readSettings(Settings& S)
{
    S.corner  = getListIndex(PIdx::corner);
    S.font    = getIntegerValue(PIdx::font);  if (S.font < 7) S.font = 7;  if (S.font > 40) S.font = 40;
    S.showA   = isBoxChecked(PIdx::showA)   != 0;
    S.showE   = isBoxChecked(PIdx::showE)   != 0;
    S.showhdr = isBoxChecked(PIdx::showhdr) != 0;
    S.bg      = isBoxChecked(PIdx::bg)      != 0;
    S.xoff    = getIntegerValue(PIdx::xoff);  if (S.xoff < 0) S.xoff = 0; if (S.xoff > 600) S.xoff = 600;
    S.yoff    = getIntegerValue(PIdx::yoff);  if (S.yoff < 0) S.yoff = 0; if (S.yoff > 600) S.yoff = 600;
}

// ---- number/format helpers ------------------------------------------------
std::string DayStats::clk(double so)              { return dsl::clk(so); }              // (v0.8) DayStatsLogic.h
std::string DayStats::dur(double m)               { return dsl::dur(m); }
std::string DayStats::px1(const std::string& raw) { return dsl::px1(raw, state().priceOff); }

void DayStats::textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short w = (short)getTextWidth(s, -1); if (w < 300) w = 300;                       // (v0.13) the READ line runs past 300 px with its arrival clock
    RCT rc; rc.set(x, (short)(y - sz), (short)(x + w + 4), (short)(y + sz));
    rc.drawText(s, false, false);   // left-justified, vertically centred on y
}
short DayStats::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (short)getTextWidth(s, -1);
}
// (v0.5) centre a string on xc (used for the READ line on the title row)
void DayStats::textCJ(short xc, short y, const char* s, COLOR col, int sz, bool bold)
{
    short w = textW(s, sz, bold);
    textLJ((short)(xc - w / 2), y, s, col, sz, bold);
}

// ---- data load ------------------------------------------------------------
void DayStats::load()
{
    State& s = state();
    const RTDATE now = currentDate();
    if (!dsl::cacheRefreshDue(now == s.lastLoadDate, s.sameDateCacheSkips)) return;
    s.lastLoadDate = now;
    s.A = StatRow(); s.E = StatRow(); s.weekday.clear(); s.daydate.clear(); s.asofSo = -1; s.condBasis.clear(); s.condLastHr = -1; s.condLast30 = -1; s.condP20 = s.condP50 = s.condP80 = -1; s.read2 = dsl::Read2();
    s.hasSpot = false; s.spotPx = 0.0f; s.priceOff = 0.0f;
    s.hasRead = false; s.readFirst.clear(); s.readCall.clear(); s.readPosr = -1; s.readPct = -1; s.readN = 0;
    s.read1 = dsl::Read2(); s.read2 = dsl::Read2();   // (v0.14) a row absent from this export must not linger from the last
    s.eOpen = -1;   // (v0.16)
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";
    std::ifstream f(path.c_str()); if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t; std::stringstream ss(line); std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.size() < 2) continue;
        const std::string tag = dsl::trim(t[0]);
        if ((tag == "DAYSA" || tag == "DAYSE") && t.size() >= 16) {
            StatRow r; if (dsl::parseStatRow(t, r)) { if (tag == "DAYSA") s.A = r; else s.E = r; }   // (v0.8) DayStatsLogic.h
        } else if (tag == "CONDE" && t.size() >= 6) {
            // (v0.7) CONDE,<basis>,<t1>,<t2>,<lod%>,<n>[,<lastHr%>,<last30%>] — which stage drew the E clocks (panel 16.32/16.35)
            dsl::Cond c; if (dsl::parseCond(t, c)) { s.condBasis = c.basis; s.condLastHr = c.lastHr; s.condLast30 = c.last30; s.condP20 = c.p20; s.condP50 = c.p50; s.condP80 = c.p80; }   // (v0.8) DayStatsLogic.h · (v0.11) + percentiles
        } else if (tag == "READ2") {
            dsl::parseRead2(t, s.read2);   // (v0.12)
        } else if (tag == "DAYEXP" && t.size() >= 2) {
            double parsed = -1.0; if (dsl::parseFinite(t[1], parsed) && parsed > 0.0) s.eOpen = parsed;   // (v0.16) DAYEXP,<open>,<eHi>,<eLo>,<eClose> — the expected candle's open, for the E row's MUD move
        } else if (tag == "READ1") {
            dsl::parseRead2(t, s.read1);   // (v0.14) the first extreme's read, same shape
        } else if (tag == "WEEKDAY" && t.size() >= 2) {
            s.weekday = dsl::trim(t[1]);
            if (t.size() >= 3) s.daydate = dsl::trim(t[2]);
        } else if (tag == "ASOF" && t.size() >= 2) {
            double parsed = -1.0; if (dsl::parseFinite(t[1], parsed) && dsl::validSod(parsed)) s.asofSo = parsed;
        } else if (tag == "SPOT" && t.size() >= 2) {
            double parsed = 0.0; if (dsl::parseFinite(t[1], parsed) && parsed > 0.0) { s.spotPx = (float)parsed; s.hasSpot = true; }
        } else if (tag == "READ" && t.size() >= 6) {
            // READ,<first HOD|LOD>,<posr%>,<cellPct or blank>,<cellN>,<call IN|NOTIN|HOLD>
            dsl::Read r; if (dsl::parseRead(t, r)) { s.readFirst = r.side; s.readPosr = r.posr; s.readPct = r.pct; s.readN = r.n; s.readCall = r.call; s.hasRead = true; }
        }
    }
    dsl::setMud(s.E, s.eOpen);   // (v0.16) the E row's MUD move: |the expected 2nd extreme - the expected candle's open|, ES points
}

// ---- render ---------------------------------------------------------------
// A two-row grid (A over E) with a column-header row above it. Columns are laid
// out at fixed x offsets computed from the widest cell, so A and E line up.
void DayStats::render(const Settings& S)
{
    State& s = state();
    if (!s.A.valid && !s.E.valid) return;

    RCT pane; pane.getPaneRect(false);
    int fs = S.font;
    short lineH = (short)(fs + 5);
    short colGap = (short)(textW("0", fs, false) * 2 + 6);

    // build the 12 columns, each: header + A cell + E cell (E prefixed '~')
    // (v0.6) the IQR column (the weekday range's P25-P75 band) is gone — operator, 2026-09-16: "remove the IQR in
    // the day stats". The band still rides in the DAYSE row (fields 16/17) for the record; it is just not drawn.
    static const int NCOL = 12;
    const char* hdr[NCOL] = { "", "1ST", "TOOK", "BOP", "WICK", "W.END", "WICK%",
                              "MUD", "MUDt", "2ND", "HL GAP", "HL RNG" };
    std::string aCell[NCOL], eCell[NCOL];

    // (v0.8) the cells come from DayStatsLogic.h — the same strings the logic test pins
    if (s.A.valid) { dsl::actualCells(s.A, s.priceOff, aCell); eCell[0] = ""; }
    if (s.E.valid) { dsl::Cond c; c.basis = s.condBasis; c.lastHr = s.condLastHr; c.last30 = s.condLast30; dsl::expectedCells(s.E, c, eCell); }

    // (v0.4) per-cell colour-coding for the ACTUAL row — operator spec: the LOD extreme reads red, the HOD
    // extreme green, and MUD by the day's phase (markup = the LOD was made first and price marked up → green;
    // markdown = the HOD was made first → red).
    COLOR aCol[NCOL]; for (int c = 0; c < NCOL; c++) aCol[c] = C_TXT;
    if (s.A.valid) { int tone[dsl::NCOL]; dsl::actualTone(s.A, tone); for (int c = 0; c < NCOL; c++) aCol[c] = tone[c] > 0 ? C_UPG : (tone[c] < 0 ? C_DNR : C_TXT); }

    // column widths = max over header / A / E
    short colW[NCOL];
    for (int c = 0; c < NCOL; c++) {
        short w = textW(hdr[c], fs, true);
        if (S.showA && s.A.valid) { short wa = textW(aCell[c].c_str(), fs, false); if (wa > w) w = wa; }
        if (S.showE && s.E.valid) { short we = textW(eCell[c].c_str(), fs, false); if (we > w) w = we; }
        colW[c] = w;
    }
    short colX[NCOL]; short totalW = 0;
    for (int c = 0; c < NCOL; c++) { colX[c] = totalW; totalW += colW[c] + colGap; }

    int rows = (S.showhdr ? 1 : 0) + (S.showA && s.A.valid ? 1 : 0) + (S.showE && s.E.valid ? 1 : 0);
    short titleH = lineH;
    short blockW = (short)(totalW + 12);
    short blockH = (short)(titleH + rows * lineH + 10);

    // anchor to the chosen corner — (v0.13) or the pane's midline (Top-centre / Bottom-centre), never starting left of
    // the pane (dsl::anchorX): the operator's first evening had the right-anchored block cut off at the left
    short x0 = (short)dsl::anchorX(S.corner, pane.left, pane.right, blockW, S.xoff);
    short y0 = dsl::anchorBottom(S.corner) ? (short)(pane.bottom - S.yoff - blockH) : (short)(pane.top + S.yoff);

    if (S.bg) { RCT bg; bg.set(x0, y0, (short)(x0 + blockW), (short)(y0 + blockH));
                bg.draw(1, C_BORDER, C_PANEL, DRAW_OPAQUE, PAT_SOLID); }

    short tx = (short)(x0 + 6);
    short y  = (short)(y0 + 4 + fs);

    // title: DAY STATS — <Weekday> <date>
    std::string title = "DAY STATS";
    if (!s.weekday.empty()) { title += " - "; title += s.weekday; if (!s.daydate.empty()) { title += " "; title += s.daydate; } }
    textLJ(tx, y, title.c_str(), C_TITLE, fs, true);
    // (v0.5) THE READ — center-justified on the DAY STATS title line (operator: "the prediction line like HOD IN /
    // LOD IN ... center justified in the line at the top where it says Day Stats"). This is the validated HLTAB
    // classifier: "has the standing extreme printed?"  IN = green (trust it), NOT IN = amber (the move isn't over).
    if (s.hasRead && !s.readFirst.empty()) {
        // (v0.12) THE SECOND HALF: the second extreme's own read (READ2, panel 16.46) — "LOD IN 80%", or "LOD IN 25% · if not,
        // ~10:09am (50%)" while it is probably still ahead; after the close, the actual 2ND from the A row. (0.11's descending
        // rungs are gone: the operator wants one rising number per extreme, not a probability that expires with the clock.)
        // (v0.13) THE WHOLE LINE is dsl::readLine: the first extreme leads, and after the close BOTH halves print their clocks.
        RTDATE nowD = currentDate(); struct tm tn; memset(&tn, 0, sizeof(tn)); getLocaltime(nowD, &tn);
        double nowSo = tn.tm_hour * 3600.0 + tn.tm_min * 60.0 + tn.tm_sec;
        bool closed = nowSo >= 54000.0 || nowSo < 30600.0;
        std::string first = s.readFirst; double firstSo = -1.0;
        if (closed && s.A.valid && (s.A.first == "HOD" || s.A.first == "LOD")) { first = s.A.first; firstSo = s.A.firstClk; }
        std::string second = (first == "HOD") ? "LOD" : (first == "LOD" ? "HOD" : "");
        dsl::Read2 r2 = s.read2; if (!r2.valid && !second.empty()) r2.side = second;
        std::string s2 = dsl::secondLine(r2, s.asofSo, closed, (s.A.valid && !r2.side.empty() && s.A.second == r2.side) ? s.A.secondClk : -1.0);
        // (v0.14) the first half from READ1 (the same model) when the panel wrote one; the HLTAB cell before 36 min
        std::string h1 = dsl::firstHalf(s.read1, first, s.readCall, s.readPct, closed, firstSo, s.asofSo);
        std::string rl = h1; if (!s2.empty()) rl += "   \xB7   " + s2;
        int tone = dsl::firstTone(s.read1, first, s.readCall, closed, firstSo);
        COLOR rcol = tone > 0 ? C_UPG : (tone < 0 ? C_HEAD : C_TITLE);
        textCJ((short)(x0 + blockW / 2), y, rl.c_str(), rcol, fs, true);
    }
    y = (short)(y + lineH);

    // header row
    if (S.showhdr) {
        for (int c = 1; c < NCOL; c++) textLJ((short)(tx + colX[c]), y, hdr[c], C_HEAD, fs, true);
        y = (short)(y + lineH);
    }
    // (v0.4) EXPECTED row on TOP, then ACTUAL below (operator-directed row order)
    // E row (expected / base rate — the reference line)
    if (S.showE && s.E.valid) {
        for (int c = 0; c < NCOL; c++) if (!eCell[c].empty()) textLJ((short)(tx + colX[c]), y, eCell[c].c_str(), C_EXP, fs, (c==0));
        y = (short)(y + lineH);
    }
    // A row (actual — colour-coded per aCol)
    if (S.showA && s.A.valid) {
        for (int c = 0; c < NCOL; c++) if (!aCell[c].empty()) textLJ((short)(tx + colX[c]), y, aCell[c].c_str(), aCol[c], fs, (c==0));
        y = (short)(y + lineH);
    }
}

// ---- draw() ---------------------------------------------------------------
int DayStats::draw(void) { syncSettings(); State& s = state(); load(); applyContractOffset(); applyChartExtremes(); render(s.cfg); drawStaleBadge(); return RTX_OK; }

// (v0.10) THE ACTUAL ROW'S PRICES ARE THE CHART'S OWN SESSION HIGH / LOW (dsl::applyChartExtremes) — the panel's prices
// were Skylit ES1's (September until the 15:16 roll) and the live-close bias made them drift all evening (7722 → 7708).
// The day: the WEEKDAY row's date when the panel wrote one, else the last bar's date. RTH bars only. When the chart has
// no RTH bar for that day (a chart of another contract, or before the open) the panel's prices stay, with the old bias.
void DayStats::applyChartExtremes()
{
    State& s = state();
    if (!s.A.valid) return;
    long n = getBarCount(); if (n < 2) return;
    RTARRAY op(barOpen), hi(barHigh), lo(barLow); RTARRAYI dt(barDateTime);
    int from = (int)n - 3000; if (from < 0) from = 0;
    std::vector<int> yy, mm, dd; std::vector<double> so; std::vector<float> oo, hh, ll;
    for (int i = from; i < (int)n; i++) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        yy.push_back(t.tm_year + 1900); mm.push_back(t.tm_mon + 1); dd.push_back(t.tm_mday);
        so.push_back(t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec); oo.push_back(op[i]); hh.push_back(hi[i]); ll.push_back(lo[i]);
    }
    int wy = yy.back(), wm = mm.back(), wd = dd.back();
    if (s.daydate.size() >= 10) { int y2, m2, d2; if (sscanf(s.daydate.c_str(), "%d-%d-%d", &y2, &m2, &d2) == 3) { wy = y2; wm = m2; wd = d2; } }
    // (v0.16) the session read is stamp-aware (DayModel 0.16's rule: on an end-stamped chart the 08:30 bar is the pre-open bar)
    // and carries the RTH OPEN, for the MUD move: |the chart's 2nd extreme - the chart's open|, in the chart's own points
    dsl::Ext X;
    bool endSt = dsl::endStampedSods(&so[0], (int)so.size());
    bool have = dsl::chartSession(&yy[0], &mm[0], &dd[0], &so[0], &oo[0], &hh[0], &ll[0], (int)yy.size(), wy, wm, wd, endSt, X);
    if (have) { dsl::applyChartExtremes(s.A, X.hi, X.lo, true); s.priceOff = 0.0f; dsl::setMud(s.A, X.open); }   // chart facts need no bias
}

// ---- (v0.3) contract offset for the displayed price labels ([1st]/[2nd]) — match the day candle: the
// prices are in the panel's cash space; shift by (this chart's last close − SPOT) so they read in the
// chart's contract. Text-only (this panel isn't price-aligned), so it just biases px1().
void DayStats::applyContractOffset()
{
    State& s = state();
    s.priceOff = 0.0f;
    if (!s.hasSpot) return;
    long n = getBarCount(); if (n < 1) return;
    RTARRAY close(barClose);
    float chartClose = close[(int)n - 1];
    if (!dsl::isFinite(chartClose) || !dsl::isFinite(s.spotPx) || !(chartClose > 0)) return;
    float off = chartClose - s.spotPx;
    if (!dsl::isFinite(off) || off < -300.0f || off > 300.0f) return;   // implausible → no offset
    s.priceOff = off;
}

// ---- (v0.2) STALE badge — ASOF,<CT sec-of-day> vs the chart clock; > ~4 min ⇒ frozen file (overnight-safe)
void DayStats::drawStaleBadge()
{
    State& s = state();
    if (!dsl::validSod(s.asofSo)) return;
    RTDATE now = currentDate(); struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime(now, &tmv);
    double localSo = tmv.tm_hour * 3600.0 + tmv.tm_min * 60.0 + tmv.tm_sec;
    double ageMin = (s.asofSo > localSo + 300.0) ? ((86400.0 - s.asofSo) + localSo) / 60.0 : (localSo - s.asofSo) / 60.0;
    if (!dsl::isFinite(ageMin) || ageMin <= 4.0) return;
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
    DayStats *p = new DayStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI);   // text strip: no INSTRUMENT_SCALE (not price-aligned)
    p->setDescription("Day model stats strip (actual vs expected), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.18");
    return p;
}
