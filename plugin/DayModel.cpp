/********************************************************************************
 *  DayModel.cpp  —  Investor/RT RTX extension  (day model — candle pair)
 *
 *  Draws the EXPECTED (ghost) + ACTUAL (developing) day candles in a reserved
 *  margin, price-aligned to the instrument axis (INSTRUMENT_SCALE). A SEPARATE
 *  indicator from the gamma profile, so its settings dialog and parameter
 *  numbering can never destabilise lsGammaProfile.
 *
 *  Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv (shared with
 *  the gamma plugin; each plugin ignores the other's rows):
 *       DAYEXP,<open>,<high>,<low>,<close>     (expected model day)
 *       DAYACT,<open>,<high>,<low>,<close>     (actual developing day)
 *
 *  v0.1 — the candle PAIR only (bodies, split wicks, colour by close-vs-open,
 *  ghost vs solid). The HOD/LOD tip labels, MUD box, swept-level ticks and the
 *  day-model statistics strip are the next increments (data already staged in
 *  the CSV: DAYHOD / DAYLOD / DAYMUD / SWEPT / WEEKDAY).
 *
 *  Parameter indices are numbered EXPLICITLY (pc++), one per control, with NO
 *  setLabelParameter section headers -- a label row shifts IRT's parameter
 *  numbering out from under the value getters and silently scrambles settings.
 *  (Same hard-won rule as the gamma plugin.)
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
static const COLOR C_UP   = 0x0033B36B;  // up candle (close >= open)  green
static const COLOR C_DN   = 0x00D1493F;  // down candle                red
static const COLOR C_TXT  = 0x00DFE7F0;  // neutral label ink
static const COLOR C_DARK = 0x00101418;  // dark ink / outline
static const COLOR C_WHT  = 0x00FFFFFF;

static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}

// ---- parameter indices ----------------------------------------------------
struct PIdx {
    int side, layout, width, gap, showexp, showact, cup, cdn, font, labels;
};
static PIdx PX;

struct Settings {
    int side, layout, width, gap, font;
    bool showexp, showact, labels;
    COLOR cup, cdn;
};

struct DayCandle { float o, h, l, c; bool valid; };

// ---------------------------------------------------------------------------
class DayModel : public cppExtension {
public:
    DayModel();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    DayCandle expC, actC;
    Settings cfg;
    int lastBar;

    void load();
    void readSettings(Settings& S);
    void render(const Settings& S);
    // helpers
    short yOf(float price);
    void vline(short x, short y1, short y2, COLOR col, PEN_STYLE ps);
    void hline(short y, short x1, short x2, COLOR col, PEN_STYLE ps);
    void rectOutline(short l, short t, short r, short b, COLOR col, PEN_STYLE ps);
    void rectFill(short l, short t, short r, short b, COLOR fill, COLOR outline);
    void textC(short cx, short y, const char* s, COLOR col, int sz, bool bold);
    void drawCandle(short cx, const DayCandle& d, bool ghost, const Settings& S);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- constructor: seed safe defaults so the first draw is valid -----------
DayModel::DayModel() : cppExtension()
{
    exp.valid = false; act.valid = false; lastBar = 0;
    cfg.side = 0;          // Left margin (per the settled §10.1 layout)
    cfg.layout = 0;        // Pair (side by side)
    cfg.width = 34;
    cfg.gap = 12;
    cfg.showexp = true;
    cfg.showact = true;
    cfg.cup = C_UP;
    cfg.cdn = C_DN;
    cfg.font = 10;
    cfg.labels = true;
}

// ---- parameter callbacks (dialog controls valid here; guard on a plausible
//      font read so a nonsense value never corrupts the cached settings) -----
int DayModel::parmsLoad(void)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) readSettings(cfg);
    return RTX_OK;
}
int DayModel::parmsApply(void)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) readSettings(cfg);
    return RTX_OK;
}
int DayModel::parmsUpdt(unsigned int)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) readSettings(cfg);
    return RTX_OK;
}

// ---- parameter panel ------------------------------------------------------
// Controls only, counted 1:1 with pc++. NO setLabelParameter (it shifts IRT's
// parameter numbering and scrambles the getters -- see the gamma plugin).
int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(14);

    const short SL = kParmAppendSameLine;
    int pc = 0;

    PX.side    = pc++; setListParameter   ("Side", 0, "Left;Right");
    PX.layout  = pc++; setListParameter   ("Layout", 0, "Pair;Overlay", 0, SL);
    PX.width   = pc++; setIntegerParameter("Candle width px", 34);
    PX.gap     = pc++; setIntegerParameter("Margin gap px", 12, 0, SL);
    PX.showexp = pc++; setBoolParameter   ("Expected candle (ghost)", true);
    PX.showact = pc++; setBoolParameter   ("Actual candle (solid)", true, SL);
    PX.cup     = pc++; setColorParameter  ("Up colour", C_UP);
    PX.cdn     = pc++; setColorParameter  ("Down colour", C_DN, 0, SL);
    PX.font    = pc++; setIntegerParameter("Font size (pt)", 10);
    PX.labels  = pc++; setBoolParameter   ("EXP/ACT labels", true, SL);
    return RTX_OK;
}

// ---- read settings --------------------------------------------------------
void DayModel::readSettings(Settings& S)
{
    S.side   = getListIndex(PX.side);
    S.layout = getListIndex(PX.layout);
    S.width  = getIntegerValue(PX.width);  if (S.width < 8)  S.width = 8;  if (S.width > 200) S.width = 200;
    S.gap    = getIntegerValue(PX.gap);    if (S.gap  < 0)   S.gap  = 0;   if (S.gap  > 600) S.gap  = 600;
    S.showexp= isBoxChecked(PX.showexp) != 0;
    S.showact= isBoxChecked(PX.showact) != 0;
    COLOR cu=(COLOR)(getIntegerValue(PX.cup)&0xFFFFFF); S.cup = cu ? cu : C_UP;
    COLOR cd=(COLOR)(getIntegerValue(PX.cdn)&0xFFFFFF); S.cdn = cd ? cd : C_DN;
    S.font   = getIntegerValue(PX.font);   if (S.font < 7)   S.font = 7;   if (S.font > 40) S.font = 40;
    S.labels = isBoxChecked(PX.labels) != 0;
}

// ---- data load ------------------------------------------------------------
void DayModel::load()
{
    expC.valid = false; actC.valid = false;
    const char* up = getenv("USERPROFILE");
    if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";
    std::ifstream f(path.c_str());
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t; std::stringstream ss(line); std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.size() < 5) continue;
        if (t[0] == "DAYEXP") {
            expC.o=(float)atof(t[1].c_str()); expC.h=(float)atof(t[2].c_str());
            expC.l=(float)atof(t[3].c_str()); expC.c=(float)atof(t[4].c_str()); expC.valid=true;
        } else if (t[0] == "DAYACT") {
            actC.o=(float)atof(t[1].c_str()); actC.h=(float)atof(t[2].c_str());
            actC.l=(float)atof(t[3].c_str()); actC.c=(float)atof(t[4].c_str()); actC.valid=true;
        }
    }
}

// ---- drawing helpers ------------------------------------------------------
short DayModel::yOf(float price)
{
    PNT p; p.set(lastBar, price);
    return p.v;
}
void DayModel::vline(short x, short y1, short y2, COLOR col, PEN_STYLE ps)
{
    setPen(col, 1, ps);
    PNT a; a.set(0, 0.0f); a.h = x; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x; b.v = y2; b.drawLineTo();
}
void DayModel::hline(short y, short x1, short x2, COLOR col, PEN_STYLE ps)
{
    setPen(col, 1, ps);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y; b.drawLineTo();
}
void DayModel::rectOutline(short l, short t, short r, short b, COLOR col, PEN_STYLE ps)
{
    hline(t, l, r, col, ps); hline(b, l, r, col, ps);
    vline(l, t, b, col, ps); vline(r, t, b, col, ps);
}
void DayModel::rectFill(short l, short t, short r, short b, COLOR fill, COLOR outline)
{
    RCT rc; rc.set(l, t, r, b);
    rc.draw(1, outline, fill, DRAW_OPAQUE, PAT_SOLID);
}
void DayModel::textC(short cx, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    int lead=0, asc=0, desc=0; getFontMetrics(&lead, &asc, &desc);
    short tw = (short)getTextWidth(s, -1);
    setTextColor(col);
    PNT tp; tp.h = (short)(cx - tw/2); tp.v = (short)(y + (asc - desc)/2); tp.drawText(s);
}

// ---- one candle -----------------------------------------------------------
// ghost = expected (dashed outline, no fill, chart shows through);
// solid = actual (filled body). Split wicks: the wick never runs through the body.
void DayModel::drawCandle(short cx, const DayCandle& d, bool ghost, const Settings& S)
{
    short oY = yOf(d.o), hY = yOf(d.h), lY = yOf(d.l), cY = yOf(d.c);
    short bodyTop = oY < cY ? oY : cY;      // higher price = smaller Y
    short bodyBot = oY < cY ? cY : oY;
    short half = (short)(S.width / 2);
    short L = (short)(cx - half), R = (short)(cx + half);
    COLOR col = (d.c >= d.o) ? S.cup : S.cdn;
    PEN_STYLE ps = ghost ? P_DASH : P_SOLID;

    // split wicks (upper: high -> body top; lower: body bottom -> low)
    if (hY < bodyTop) vline(cx, hY, bodyTop, col, ps);
    if (lY > bodyBot) vline(cx, bodyBot, lY, col, ps);

    // body
    if (bodyBot - bodyTop < 2) {
        // doji: open == close -> a flat cross-bar
        hline(bodyTop, L, R, col, ps);
    } else if (ghost) {
        rectOutline(L, bodyTop, R, bodyBot, col, P_DASH);   // ghost = outline only
    } else {
        rectFill(L, bodyTop, R, bodyBot, col, lerpColor(col, C_DARK, 0.45f));
    }

    // EXP / ACT label just under the low
    if (S.labels) {
        textC(cx, (short)(lY + S.font + 3), ghost ? "EXP" : "ACT", col, S.font - 1, true);
    }
}

// ---- render ---------------------------------------------------------------
void DayModel::render(const Settings& S)
{
    if (!expC.valid && !actC.valid) return;
    long n = getBarCount(); if (n < 1) return;
    lastBar = (int)n - 1;

    RCT pane; pane.getPaneRect(false);
    short paneL = pane.left, paneR = pane.right;

    short half = (short)(S.width / 2);
    short inner = 8;                         // gap between the two candles in Pair layout

    short expCx, actCx;
    if (S.layout == 1) {                     // Overlay: same column
        short cx = (S.side == 1) ? (short)(paneR - S.gap - half) : (short)(paneL + S.gap + half);
        expCx = cx; actCx = cx;
    } else {                                 // Pair: two columns; ACT nearest price
        if (S.side == 1) {                   // Right margin: ACT on the right
            actCx = (short)(paneR - S.gap - half);
            expCx = (short)(actCx - S.width - inner);
        } else {                             // Left margin (default): ACT on the right of the pair
            expCx = (short)(paneL + S.gap + half);
            actCx = (short)(expCx + S.width + inner);
        }
    }

    // Expected behind, Actual on top (matters in Overlay).
    if (S.showexp && expC.valid) drawCandle(expCx, expC, true,  S);
    if (S.showact && actC.valid) drawCandle(actCx, actC, false, S);
}

// ---- draw() ---------------------------------------------------------------
int DayModel::draw(void)
{
    load();
    render(cfg);
    return RTX_OK;
}

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    DayModel *p = new DayModel();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("Day model candle (expected + actual), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.1");
    return p;
}
