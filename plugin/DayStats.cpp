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
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// ---- palette --------------------------------------------------------------
static const COLOR C_TXT   = 0x00DFE7F0;  // actual (A) ink            near-white
static const COLOR C_EXP   = 0x009AB0C4;  // expected (E) ink          dim slate
static const COLOR C_HEAD  = 0x00E0A030;  // column headers            amber
static const COLOR C_TITLE = 0x00FFFFFF;  // panel title               white
static const COLOR C_PANEL = 0x00141A22;  // background panel fill     dark slate
static const COLOR C_BORDER= 0x00394654;  // panel border              slate grey
static const COLOR C_GOOD  = 0x0033B36B;  // A beats/near E where relevant (unused v0.1)
static const COLOR C_RULE  = 0x00263039;  // faint row rule

// ---- parameter indices ----------------------------------------------------
struct PIdx { int corner, showA, showE, showhdr, bg, font, xoff, yoff; };
static PIdx PX;

struct Settings {
    int corner;             // 0 TL, 1 TR, 2 BL, 3 BR
    bool showA, showE, showhdr, bg;
    int font, xoff, yoff;
};

// one parsed stats row (raw strings; formatted at render)
struct StatRow {
    bool valid;
    std::string first, second;
    std::string firstPx, secondPx;
    double firstClk, secondClk, wendSo;   // secOfDay (<0 = missing)
    double took, bop, wick, mud, gap;     // minutes (<0 = missing)
    double wickPct;                       // percent (<0 = missing)
    std::string rngPts, rngUsd, rngP25, rngP75;
    StatRow() : valid(false), firstClk(-1), secondClk(-1), wendSo(-1),
                took(-1), bop(-1), wick(-1), mud(-1), gap(-1), wickPct(-1) {}
};

// ---------------------------------------------------------------------------
class DayStats : public cppExtension {
public:
    DayStats();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    StatRow A, E;
    std::string weekday, daydate;
    Settings cfg;

    void load();
    void readSettings(Settings& S);
    void render(const Settings& S);
    // helpers
    void textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold);
    short textW(const char* s, int sz, bool bold);
    static double num(const std::string& s);   // "" -> -1
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

// ---- constructor: safe defaults so the first draw is valid ----------------
DayStats::DayStats() : cppExtension()
{
    cfg.corner  = 0;     // top-left
    cfg.showA   = true;
    cfg.showE   = true;
    cfg.showhdr = true;
    cfg.bg      = true;
    cfg.font    = 11;
    cfg.xoff    = 8;
    cfg.yoff    = 8;
}

// ---- parameter callbacks (guard on a plausible font read) -----------------
int DayStats::parmsLoad(void)  { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }
int DayStats::parmsApply(void) { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }
int DayStats::parmsUpdt(unsigned int) { int p=getIntegerValue(PX.font); if (p>=6 && p<=48) readSettings(cfg); return RTX_OK; }

// ---- parameter panel ------------------------------------------------------
int cppExtension::setup(void)
{
    setParameterVersion(1);            // v0.1 first release — establishes the defaults below
    setParameterDialogHeight(12);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.corner  = pc++; setListParameter   ("Corner", 0, "Top-left;Top-right;Bottom-left;Bottom-right");
    PX.font    = pc++; setIntegerParameter("Font size (pt)", 11, 0, SL);
    PX.showA   = pc++; setBoolParameter   ("Actual row (A)", true);
    PX.showE   = pc++; setBoolParameter   ("Expected row (E)", true, SL);
    PX.showhdr = pc++; setBoolParameter   ("Column headers", true);
    PX.bg      = pc++; setBoolParameter   ("Background panel", true, SL);
    PX.xoff    = pc++; setIntegerParameter("X inset px", 8, 0);
    PX.yoff    = pc++; setIntegerParameter("Y inset px", 8, 0, SL);
    return RTX_OK;
}

void DayStats::readSettings(Settings& S)
{
    S.corner  = getListIndex(PX.corner);
    S.font    = getIntegerValue(PX.font);  if (S.font < 7) S.font = 7;  if (S.font > 40) S.font = 40;
    S.showA   = isBoxChecked(PX.showA)   != 0;
    S.showE   = isBoxChecked(PX.showE)   != 0;
    S.showhdr = isBoxChecked(PX.showhdr) != 0;
    S.bg      = isBoxChecked(PX.bg)      != 0;
    S.xoff    = getIntegerValue(PX.xoff);  if (S.xoff < 0) S.xoff = 0; if (S.xoff > 600) S.xoff = 600;
    S.yoff    = getIntegerValue(PX.yoff);  if (S.yoff < 0) S.yoff = 0; if (S.yoff > 600) S.yoff = 600;
}

// ---- number/format helpers ------------------------------------------------
double DayStats::num(const std::string& s) { if (s.empty()) return -1; return atof(s.c_str()); }

std::string DayStats::clk(double so)
{
    if (so < 0) return "--";
    int s = (int)(so + 0.5);
    int h = s / 3600, m = (s % 3600) / 60;
    const char* ap = (h < 12) ? "am" : "pm";
    int h12 = h % 12; if (h12 == 0) h12 = 12;
    char b[16]; sprintf_s(b, sizeof(b), "%d:%02d%s", h12, m, ap);
    return std::string(b);
}
std::string DayStats::dur(double m)
{
    if (m < 0) return "--";
    int mins = (int)(m + 0.5);
    char b[16];
    if (mins >= 60) sprintf_s(b, sizeof(b), "%dh %02dm", mins / 60, mins % 60);
    else            sprintf_s(b, sizeof(b), "%dm", mins);
    return std::string(b);
}
std::string DayStats::px1(const std::string& raw)
{
    if (raw.empty()) return "--";
    double v = atof(raw.c_str());
    char b[16]; sprintf_s(b, sizeof(b), "%d", (int)(v + 0.5));
    return std::string(b);
}

void DayStats::textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    RCT rc; rc.set(x, (short)(y - sz), (short)(x + 300), (short)(y + sz));
    rc.drawText(s, false, false);   // left-justified, vertically centred on y
}
short DayStats::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (short)getTextWidth(s, -1);
}

// ---- data load ------------------------------------------------------------
void DayStats::load()
{
    A = StatRow(); E = StatRow(); weekday.clear(); daydate.clear();
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";
    std::ifstream f(path.c_str()); if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t; std::stringstream ss(line); std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.size() < 2) continue;
        if ((t[0] == "DAYSA" || t[0] == "DAYSE") && t.size() >= 16) {
            StatRow r;
            r.first     = t[1];
            r.firstPx   = t[2];
            r.firstClk  = num(t[3]);
            r.took      = num(t[4]);
            r.bop       = num(t[5]);
            r.wick      = num(t[6]);
            r.wendSo    = num(t[7]);
            r.wickPct   = num(t[8]);
            r.mud       = num(t[9]);
            r.second    = t[10];
            r.secondPx  = t[11];
            r.secondClk = num(t[12]);
            r.gap       = num(t[13]);
            r.rngPts    = t[14];
            r.rngUsd    = t[15];
            r.rngP25    = (t.size() > 16) ? t[16] : "";
            r.rngP75    = (t.size() > 17) ? t[17] : "";
            r.valid     = true;
            if (t[0] == "DAYSA") A = r; else E = r;
        } else if (t[0] == "WEEKDAY" && t.size() >= 2) {
            weekday = t[1];
            if (t.size() >= 3) daydate = t[2];
        }
    }
}

// ---- render ---------------------------------------------------------------
// A two-row grid (A over E) with a column-header row above it. Columns are laid
// out at fixed x offsets computed from the widest cell, so A and E line up.
void DayStats::render(const Settings& S)
{
    if (!A.valid && !E.valid) return;

    RCT pane; pane.getPaneRect(false);
    int fs = S.font;
    short lineH = (short)(fs + 5);
    short colGap = (short)(textW("0", fs, false) * 2 + 6);

    // build the 14 columns, each: header + A cell + E cell (E prefixed '~')
    static const int NCOL = 14;
    const char* hdr[NCOL] = { "", "1ST", "TOOK", "BOP", "WICK", "W.END", "WICK%",
                              "MUD", "MUDt", "2ND", "HL GAP", "HL RNG$", "HL RNGp", "IQR" };
    std::string aCell[NCOL], eCell[NCOL];

    // A cells
    if (A.valid) {
        double aMudt = (A.gap >= 0 && A.bop >= 0) ? (A.gap - A.bop) : -1;
        aCell[0]="A"; eCell[0]="";
        aCell[1]= A.first + " " + clk(A.firstClk) + " " + px1(A.firstPx);
        aCell[2]= dur(A.took);
        aCell[3]= dur(A.bop);
        aCell[4]= dur(A.wick);
        aCell[5]= clk(A.wendSo);
        aCell[6]= (A.wickPct>=0) ? (std::to_string((int)(A.wickPct+0.5))+"%") : "--";
        aCell[7]= dur(A.mud);
        aCell[8]= dur(aMudt);
        aCell[9]= A.second + " " + clk(A.secondClk) + " " + px1(A.secondPx);
        aCell[10]= dur(A.gap);
        aCell[11]= A.rngUsd.empty()?"--":("$"+A.rngUsd);
        aCell[12]= A.rngPts.empty()?"--":(A.rngPts+"p");
        aCell[13]= "";
    }
    // E cells (base-rate medians; '~' marks them)
    if (E.valid) {
        double eMudt = (E.gap >= 0 && E.bop >= 0) ? (E.gap - E.bop) : -1;
        eCell[0]="E";
        eCell[1]= E.first + " ~" + clk(E.firstClk);
        eCell[2]= "~"+dur(E.took);
        eCell[3]= "~"+dur(E.bop);
        eCell[4]= "~"+dur(E.wick);
        eCell[5]= "~"+clk(E.wendSo);
        eCell[6]= (E.wickPct>=0) ? ("~"+std::to_string((int)(E.wickPct+0.5))+"%") : "--";
        eCell[7]= "~"+dur(E.mud);
        eCell[8]= "~"+dur(eMudt);
        eCell[9]= E.second + " ~" + clk(E.secondClk);
        eCell[10]= "~"+dur(E.gap);
        eCell[11]= E.rngUsd.empty()?"--":("~$"+E.rngUsd);
        eCell[12]= E.rngPts.empty()?"--":("~"+E.rngPts+"p");
        eCell[13]= (!E.rngP25.empty() && !E.rngP75.empty()) ? (E.rngP25+"-"+E.rngP75) : "";
    }

    // column widths = max over header / A / E
    short colW[NCOL];
    for (int c = 0; c < NCOL; c++) {
        short w = textW(hdr[c], fs, true);
        if (S.showA && A.valid) { short wa = textW(aCell[c].c_str(), fs, false); if (wa > w) w = wa; }
        if (S.showE && E.valid) { short we = textW(eCell[c].c_str(), fs, false); if (we > w) w = we; }
        colW[c] = w;
    }
    short colX[NCOL]; short totalW = 0;
    for (int c = 0; c < NCOL; c++) { colX[c] = totalW; totalW += colW[c] + colGap; }

    int rows = (S.showhdr ? 1 : 0) + (S.showA && A.valid ? 1 : 0) + (S.showE && E.valid ? 1 : 0);
    short titleH = lineH;
    short blockW = (short)(totalW + 12);
    short blockH = (short)(titleH + rows * lineH + 10);

    // anchor to the chosen corner
    short x0, y0;
    switch (S.corner) {
        case 1:  x0 = (short)(pane.right - S.xoff - blockW); y0 = (short)(pane.top + S.yoff); break;   // TR
        case 2:  x0 = (short)(pane.left  + S.xoff);          y0 = (short)(pane.bottom - S.yoff - blockH); break; // BL
        case 3:  x0 = (short)(pane.right - S.xoff - blockW); y0 = (short)(pane.bottom - S.yoff - blockH); break; // BR
        default: x0 = (short)(pane.left  + S.xoff);          y0 = (short)(pane.top + S.yoff); break;   // TL
    }

    if (S.bg) { RCT bg; bg.set(x0, y0, (short)(x0 + blockW), (short)(y0 + blockH));
                bg.draw(1, C_BORDER, C_PANEL, DRAW_OPAQUE, PAT_SOLID); }

    short tx = (short)(x0 + 6);
    short y  = (short)(y0 + 4 + fs);

    // title: DAY STATS — <Weekday> <date>
    std::string title = "DAY STATS";
    if (!weekday.empty()) { title += " - "; title += weekday; if (!daydate.empty()) { title += " "; title += daydate; } }
    textLJ(tx, y, title.c_str(), C_TITLE, fs, true);
    y = (short)(y + lineH);

    // header row
    if (S.showhdr) {
        for (int c = 1; c < NCOL; c++) textLJ((short)(tx + colX[c]), y, hdr[c], C_HEAD, fs, true);
        y = (short)(y + lineH);
    }
    // A row
    if (S.showA && A.valid) {
        for (int c = 0; c < NCOL; c++) if (!aCell[c].empty()) textLJ((short)(tx + colX[c]), y, aCell[c].c_str(), C_TXT, fs, (c==0));
        y = (short)(y + lineH);
    }
    // E row
    if (S.showE && E.valid) {
        for (int c = 0; c < NCOL; c++) if (!eCell[c].empty()) textLJ((short)(tx + colX[c]), y, eCell[c].c_str(), C_EXP, fs, (c==0));
        y = (short)(y + lineH);
    }
}

// ---- draw() ---------------------------------------------------------------
int DayStats::draw(void) { load(); render(cfg); return RTX_OK; }

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    DayStats *p = new DayStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI);   // text strip: no INSTRUMENT_SCALE (not price-aligned)
    p->setDescription("Day model stats strip (actual vs expected), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.1");
    return p;
}
