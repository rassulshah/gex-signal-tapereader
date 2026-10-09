/********************************************************************************
 *  DayModel.cpp  —  Investor/RT RTX extension  (day model — the candle)
 *
 *  Draws the EXPECTED (ghost) + ACTUAL (developing) day candles in a reserved
 *  margin, price-aligned to the instrument axis (INSTRUMENT_SCALE). A SEPARATE
 *  indicator from the gamma profile, so its settings dialog and parameter
 *  numbering can never destabilise lsGammaProfile.
 *
 *  Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv (shared with
 *  the gamma plugin; each plugin ignores the other's rows):
 *       DAYEXP,<o>,<h>,<l>,<c>              expected model day
 *       DAYACT,<o>,<h>,<l>,<c>              actual developing day
 *       DAYHOD,<value>,<time>,<duration>    actual HOD  (tip label)
 *       DAYLOD,<value>,<time>,<duration>    actual LOD  (tip label)
 *       DAYMUD,<pts>,<time>,<dollar>        MUD box inside the body
 *       SWEPT,<name>,<price>,<time>,<R|B|T> swept level (dashed tick + tag)
 *       WEEKDAY,<Mon..Fri>                  day-of-week header
 *
 *  v0.4 — bodies, split wicks, colour by close-vs-open, ghost vs solid; actual
 *  HOD/LOD tips (value/time/duration) + MUD box + swept ticks; expected candle
 *  E-HOD / E-LOD / E-C tips; background panel; adjustable EXP<->ACT spacing;
 *  day-of-week header; optional E-HOD/E-LOD reference lines. Stats strip next.
 *  v0.14 — STALE GUARD: past 15 min since ASOF the frozen EXPECTED candle stops
 *  drawing (it and its E-lines/annotations), so a cold overnight CSV no longer
 *  prints a huge mis-aligned candle. Actual candle (chart-measured) still draws;
 *  STALE badge still shown. Age computed once in staleAgeMin() for badge+guard.
 *  v0.15 — measureChartDay measures the RTH DAY (08:30-15:00) of the most recent
 *  RTH day, not IRT's rolling session, so the actual candle's low no longer jumps
 *  to the evening session after the close (the 7652-vs-7643.50 mismatch).
 *  v0.20 — review correction: store every mutable chart value in current-context SDK
 *  user data; a shared DLL object no longer lets one chart overwrite another.
 *
 *  Parameter indices are numbered EXPLICITLY (pc++), one per control, with NO
 *  setLabelParameter section headers -- a label row shifts IRT's parameter
 *  numbering out from under the value getters and silently scrambles settings.
 ********************************************************************************/
// Avoid legacy Windows min/max/far macros colliding with this translation unit.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "irtsdk.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef far
#undef far
#endif
#include "DayModelLogic.h"
#include "HostSlot.h"   // (v0.17) the decisions, testable without IRT — plugin/test_daymodel_logic.cpp
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
static const COLOR C_UP    = 0x0033B36B;  // up candle / swept-reclaimed   green
static const COLOR C_DN    = 0x00D1493F;  // down candle / swept-broke      red
static const COLOR C_TXT   = 0x00DFE7F0;  // neutral label ink
static const COLOR C_DARK  = 0x00101418;  // dark ink / outline
static const COLOR C_WHT   = 0x00FFFFFF;
static const COLOR C_AMBER = 0x00E0A030;  // swept level being tested      amber
static const COLOR C_PANEL = 0x00141A22;  // background panel fill          dark slate
static const COLOR C_BORDER= 0x00394654;  // panel border                   slate grey
static const COLOR C_ELINE = 0x00356E78;  // E-HOD/E-LOD reference line     dim teal

static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}

// ---- parameter indices ----------------------------------------------------
struct PIdx {
    int side, layout, width, spacing, gap, bg, showexp, showact, cup, cdn;
    int hilo, mud, swept, header, elines, font, labels;
};
static PIdx PX;

struct Settings {
    int side, layout, width, spacing, gap, font;
    bool bg, showexp, showact, labels, hilo, mud, swept, header, elines;
    COLOR cup, cdn;
};

struct DayCandle { float o, h, l, c; bool valid; };
struct SweptLvl  { float price; std::string name, time; char state; };

// cppExtension can be a single DLL object serving several chart contexts.  Every
// mutable chart value must therefore live behind getUserData()/setUserData(), not
// in DayModel itself.  The SDK associates that pointer with the current host
// context and calls done() when its symbol/period changes and destroy() on removal.
struct DayModelState {
    DayCandle expC, actC;
    float hodV, lodV; std::string hodT, hodD, lodT, lodD; bool hasHod, hasLod;
    std::string mudP, mudT, mudDol; bool hasMud;
    std::string ehodT, ehodD, elodT, elodD; bool hasEHodT, hasELodT;
    std::string emudP, emudT, emudDol; bool hasEMud;
    std::vector<SweptLvl> swepts;
    std::string weekday, daydate;
    double asofSo;
    float spotPx; bool hasSpot;
    Settings cfg;
    int lastBar;
    bool endStamped;

    DayModelState()
        : hodV(0.0f), lodV(0.0f), hasHod(false), hasLod(false), hasMud(false),
          hasEHodT(false), hasELodT(false), hasEMud(false), asofSo(-1.0),
          spotPx(0.0f), hasSpot(false), lastBar(0), endStamped(false)
    {
        expC.o = expC.h = expC.l = expC.c = 0.0f; expC.valid = false;
        actC.o = actC.h = actC.l = actC.c = 0.0f; actC.valid = false;
        cfg.side = 0; cfg.layout = 0; cfg.width = 46; cfg.spacing = 24; cfg.gap = 12;
        cfg.bg = false; cfg.showexp = true; cfg.showact = true; cfg.labels = false;
        cfg.hilo = true; cfg.mud = true; cfg.swept = true; cfg.header = true; cfg.elines = false;
        cfg.cup = C_UP; cfg.cdn = C_DN; cfg.font = 10;
    }
};

// ---------------------------------------------------------------------------
class DayModel : public cppExtension {
public:
    DayModel();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void);

    DayModelState* getState(bool create);
    HostSlot<DayModelState> slot_;
    void clearState();
    void load(DayModelState& state);
    double staleAgeMin(const DayModelState& state);
    void drawStaleBadge(const DayModelState& state);
    void applyContractOffset(DayModelState& state, const std::vector<dml::Bar>& bars);
    bool measureChartDay(DayModelState& state, const std::vector<dml::Bar>& bars);
    void computeChartLevels(DayModelState& state, const std::vector<dml::Bar>& bars);
    void detectBarStamp(DayModelState& state, const std::vector<dml::Bar>& bars);
    void loadBars(std::vector<dml::Bar>& out, int maxBars);
    void readSettings(Settings& S);
    void render(DayModelState& state, const Settings& S);
    short yOf(const DayModelState& state, float price);
    void vline(short x, short y1, short y2, COLOR col, PEN_STYLE ps);
    void hline(short y, short x1, short x2, COLOR col, PEN_STYLE ps);
    void rectOutline(short l, short t, short r, short b, COLOR col, PEN_STYLE ps);
    void rectFill(short l, short t, short r, short b, COLOR fill, COLOR outline);
    void textC(short cx, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold);
    void drawCandle(const DayModelState& state, short cx, const DayCandle& d, bool ghost, const Settings& S);
    void drawActExtras(const DayModelState& state, short cx, const Settings& S);
    void drawExpExtras(const DayModelState& state, short cx, const Settings& S);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- current-context persistent state and lifecycle -----------------------
DayModel::DayModel() : cppExtension() {}

DayModelState* DayModel::getState(bool create)
{
    return slot_.get(this, create);
}

void DayModel::clearState()
{
    slot_.release(this);
}

int DayModel::done(void)
{
    clearState();
    return RTX_OK;
}

int DayModel::destroy(void)
{
    clearState();
    return RTX_OK;
}

// ---- parameter callbacks (dialog controls valid here; guard on a plausible
//      font read so a nonsense value never corrupts the cached settings) -----
int DayModel::parmsLoad(void)
{
    DayModelState* state = getState(true);
    if (!state) return RTX_FAIL;
    int probe = getIntegerValue(PX.font);
    if (probe >= 1 && probe <= 200 /* (2026-09-17) was 6..48: at Font size 3 no dialog change ever applied (KT 0.12 lesson) */) readSettings(state->cfg);
    return RTX_OK;
}
int DayModel::parmsApply(void)
{
    DayModelState* state = getState(true);
    if (!state) return RTX_FAIL;
    int probe = getIntegerValue(PX.font);
    if (probe >= 1 && probe <= 200 /* (2026-09-17) was 6..48: at Font size 3 no dialog change ever applied (KT 0.12 lesson) */) readSettings(state->cfg);
    return RTX_OK;
}
int DayModel::parmsUpdt(unsigned int)
{
    DayModelState* state = getState(true);
    if (!state) return RTX_FAIL;
    int probe = getIntegerValue(PX.font);
    if (probe >= 1 && probe <= 200 /* (2026-09-17) was 6..48: at Font size 3 no dialog change ever applied (KT 0.12 lesson) */) readSettings(state->cfg);
    return RTX_OK;
}

// ---- parameter panel ------------------------------------------------------
// Controls only, counted 1:1 with pc++. NO setLabelParameter (it shifts IRT's
// parameter numbering and scrambles the getters -- see the gamma plugin).
int cppExtension::setup(void)
{
    setParameterVersion(6);           // (v0.7) BUMPED to reset persisted state -> the correct defaults:
                                      // Expected+Actual candles ON, MUD box ON, Swept levels ON,
                                      // Up colour green / Down colour red. IRT re-applies the setX
                                      // defaults below whenever this number changes.
    setParameterDialogHeight(22);

    const short SL = kParmAppendSameLine;
    int pc = 0;

    PX.side    = pc++; setListParameter   ("Side", 0, "Left;Right");
    PX.layout  = pc++; setListParameter   ("Layout", 0, "Pair;Overlay", 0, SL);
    PX.width   = pc++; setIntegerParameter("Candle width px", 46, 0);
    PX.spacing = pc++; setIntegerParameter("Pair spacing px", 24, 0, SL);
    PX.gap     = pc++; setIntegerParameter("Margin gap px", 12, 0);
    PX.bg      = pc++; setBoolParameter   ("Background fill", false, SL);
    PX.showexp = pc++; setBoolParameter   ("Expected candle (ghost)", true);
    PX.showact = pc++; setBoolParameter   ("Actual candle (solid)", true, SL);
    PX.cup     = pc++; setColorParameter  ("Up colour", C_UP);
    PX.cdn     = pc++; setColorParameter  ("Down colour", C_DN, 0, SL);
    PX.hilo    = pc++; setBoolParameter   ("HOD/LOD tips", true);
    PX.mud     = pc++; setBoolParameter   ("MUD box", true, SL);
    PX.swept   = pc++; setBoolParameter   ("Swept levels", true, SL);
    PX.header  = pc++; setBoolParameter   ("Day header", true);
    PX.elines  = pc++; setBoolParameter   ("E-HOD/E-LOD lines", false, SL);
    PX.font    = pc++; setIntegerParameter("Font size (pt)", 10, 0);
    PX.labels  = pc++; setBoolParameter   ("EXP/ACT tags", false, SL);
    return RTX_OK;
}

// ---- read settings --------------------------------------------------------
void DayModel::readSettings(Settings& S)
{
    S.side   = getListIndex(PX.side);
    S.layout = getListIndex(PX.layout);
    S.width  = getIntegerValue(PX.width);    if (S.width < 8)   S.width = 8;   if (S.width > 200)  S.width = 200;
    S.spacing= getIntegerValue(PX.spacing);  if (S.spacing < 0) S.spacing = 0; if (S.spacing > 400) S.spacing = 400;
    S.gap    = getIntegerValue(PX.gap);      if (S.gap  < 0)    S.gap  = 0;    if (S.gap  > 600)   S.gap  = 600;
    S.bg     = isBoxChecked(PX.bg) != 0;
    S.showexp= isBoxChecked(PX.showexp) != 0;
    S.showact= isBoxChecked(PX.showact) != 0;
    COLOR cu=(COLOR)(getIntegerValue(PX.cup)&0xFFFFFF); S.cup = cu ? cu : C_UP;
    COLOR cd=(COLOR)(getIntegerValue(PX.cdn)&0xFFFFFF); S.cdn = cd ? cd : C_DN;
    S.hilo   = isBoxChecked(PX.hilo)   != 0;
    S.mud    = isBoxChecked(PX.mud)    != 0;
    S.swept  = isBoxChecked(PX.swept)  != 0;
    S.header = isBoxChecked(PX.header) != 0;
    S.elines = isBoxChecked(PX.elines) != 0;
    S.font   = getIntegerValue(PX.font);     if (S.font < 7)    S.font = 7;    if (S.font > 40)    S.font = 40;
    S.labels = isBoxChecked(PX.labels) != 0;
}

// ---- data load ------------------------------------------------------------
void DayModel::load(DayModelState& state)
{
    state.expC.valid = false; state.actC.valid = false;
    state.hasHod = false; state.hasLod = false; state.hasMud = false; state.swepts.clear(); state.weekday.clear(); state.daydate.clear();
    state.hasEHodT = false; state.hasELodT = false; state.hasEMud = false; state.asofSo = -1; state.hasSpot = false; state.spotPx = 0.0f;
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
        if (t.size() < 2) continue;
        if (t[0] == "DAYEXP" && t.size() >= 5) {
            float o, h, l, c;
            if (dml::parseFiniteFloat(t[1].c_str(), o) && dml::parseFiniteFloat(t[2].c_str(), h) &&
                dml::parseFiniteFloat(t[3].c_str(), l) && dml::parseFiniteFloat(t[4].c_str(), c) &&
                dml::validOhlc(o, h, l, c)) {
                state.expC.o = o; state.expC.h = h; state.expC.l = l; state.expC.c = c; state.expC.valid = true;
            }
        } else if (t[0] == "DAYACT" && t.size() >= 5) {
            float o, h, l, c;
            if (dml::parseFiniteFloat(t[1].c_str(), o) && dml::parseFiniteFloat(t[2].c_str(), h) &&
                dml::parseFiniteFloat(t[3].c_str(), l) && dml::parseFiniteFloat(t[4].c_str(), c) &&
                dml::validOhlc(o, h, l, c)) {
                state.actC.o = o; state.actC.h = h; state.actC.l = l; state.actC.c = c; state.actC.valid = true;
            }
        } else if (t[0] == "DAYHOD" && t.size() >= 4) {
            float value;
            if (dml::parseFiniteFloat(t[1].c_str(), value)) { state.hodV = value; state.hodT = t[2]; state.hodD = t[3]; state.hasHod = true; }
        } else if (t[0] == "DAYLOD" && t.size() >= 4) {
            float value;
            if (dml::parseFiniteFloat(t[1].c_str(), value)) { state.lodV = value; state.lodT = t[2]; state.lodD = t[3]; state.hasLod = true; }
        } else if (t[0] == "DAYMUD" && t.size() >= 4) {
            state.mudP=t[1]; state.mudT=t[2]; state.mudDol=t[3]; state.hasMud=true;
        } else if (t[0] == "SWEPT" && t.size() >= 5) {
            float price;
            if (dml::parseFiniteFloat(t[2].c_str(), price)) {
                SweptLvl s; s.name = t[1]; s.price = price; s.time = t[3];
                s.state = t[4].empty() ? 'T' : t[4][0];
                if (s.state != 'R' && s.state != 'B' && s.state != 'T') s.state = 'T';
                state.swepts.push_back(s);
            }
        } else if (t[0] == "DAYEHOD" && t.size() >= 4) {
            state.ehodT=t[2]; state.ehodD=t[3]; state.hasEHodT=true;
        } else if (t[0] == "DAYELOD" && t.size() >= 4) {
            state.elodT=t[2]; state.elodD=t[3]; state.hasELodT=true;
        } else if (t[0] == "DAYEMUD" && t.size() >= 4) {
            state.emudP=t[1]; state.emudT=t[2]; state.emudDol=t[3]; state.hasEMud=true;
        } else if (t[0] == "WEEKDAY" && t.size() >= 2) {
            state.weekday = t[1];
            if (t.size() >= 3) state.daydate = t[2];   // (v0.7) optional date, e.g. "11 Sep"
        } else if (t[0] == "ASOF" && t.size() >= 2) {
            double value;
            if (dml::parseFiniteDouble(t[1].c_str(), value) && dml::validSecondsOfDay(value)) state.asofSo = value;
        } else if (t[0] == "SPOT" && t.size() >= 2) {
            float value;
            if (dml::parseFiniteFloat(t[1].c_str(), value)) { state.spotPx = value; state.hasSpot = true; }
        }
    }
}

// ---- drawing helpers ------------------------------------------------------
short DayModel::yOf(const DayModelState& state, float price)
{
    PNT p; p.set(state.lastBar, price);
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
void DayModel::textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    RCT rc; rc.set(leftX, (short)(y - sz), (short)(leftX + 220), (short)(y + sz));
    rc.drawText(s, false, false);   // left-justified, vertically centred on y
}

// ---- one candle -----------------------------------------------------------
// ghost = expected (dashed outline, no fill, chart shows through);
// solid = actual (filled body). Split wicks: the wick never runs through the body.
void DayModel::drawCandle(const DayModelState& state, short cx, const DayCandle& d, bool ghost, const Settings& S)
{
    short oY = yOf(state, d.o), hY = yOf(state, d.h), lY = yOf(state, d.l), cY = yOf(state, d.c);
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
        hline(bodyTop, L, R, col, ps);                      // doji: flat cross-bar
    } else if (ghost) {
        rectOutline(L, bodyTop, R, bodyBot, col, P_DASH);   // ghost = outline only
    } else {
        rectFill(L, bodyTop, R, bodyBot, col, lerpColor(col, C_DARK, 0.45f));
    }

    // optional EXP / ACT tag (off by default)
    if (S.labels) {
        textC(cx, (short)(lY + S.font + 3), ghost ? "EXP" : "ACT", col, S.font - 1, true);
    }
}

// ---- actual-candle extras: HOD/LOD tips, MUD box, swept ticks -------------
void DayModel::drawActExtras(const DayModelState& state, short cx, const Settings& S)
{
    if (!state.actC.valid) return;
    short half = (short)(S.width / 2);
    short L = (short)(cx - half), R = (short)(cx + half);
    int fs = S.font - 1; if (fs < 7) fs = 7;
    short step = (short)(fs + 3);

    // HOD tip: value nearest the high tip, then time, then duration, stacked upward
    if (S.hilo && state.hasHod) {
        short hY = yOf(state, state.actC.h);
        char v[24]; sprintf_s(v, sizeof(v), "HOD %d", (int)(state.hodV + 0.5f));
        textC(cx, (short)(hY - step),     v,            C_TXT, fs, true);
        textC(cx, (short)(hY - 2*step),   state.hodT.c_str(), C_TXT, fs, false);
        textC(cx, (short)(hY - 3*step),   state.hodD.c_str(), C_TXT, fs, false);
    }
    // LOD tip: stacked downward below the low tip
    if (S.hilo && state.hasLod) {
        short lY = yOf(state, state.actC.l);
        char v[24]; sprintf_s(v, sizeof(v), "LOD %d", (int)(state.lodV + 0.5f));
        textC(cx, (short)(lY + step),     v,            C_TXT, fs, true);
        textC(cx, (short)(lY + 2*step),   state.lodT.c_str(), C_TXT, fs, false);
        textC(cx, (short)(lY + 3*step),   state.lodD.c_str(), C_TXT, fs, false);
    }
    // MUD box inside the body: label / points / time / dollar
    // (v0.13) operator: "MUD should be on another line above the number of points" — label and value split.
    if (S.mud && state.hasMud) {
        short oY = yOf(state, state.actC.o), cY = yOf(state, state.actC.c);
        short mid = (short)((oY + cY) / 2);
        char d[24]; sprintf_s(d, sizeof(d), "$%s", state.mudDol.c_str());
        textC(cx, (short)(mid - 2*step), "MUD",          C_WHT, fs, true);   // label on its own line, above the points
        textC(cx, (short)(mid - step),   state.mudP.c_str(),   C_WHT, fs, true);   // the number of points
        textC(cx, mid,                   state.mudT.c_str(),   C_WHT, fs, false);
        textC(cx, (short)(mid + step),   d,              C_WHT, fs, false);
    }
    // swept levels: a dashed tick across the candle + a tag to the RIGHT
    if (S.swept) {
        for (size_t i = 0; i < state.swepts.size(); i++) {
            const SweptLvl& s = state.swepts[i];
            short y = yOf(state, s.price);
            COLOR sc = (s.state=='R') ? C_UP : (s.state=='B') ? C_DN : C_AMBER;
            hline(y, L, R, sc, P_DOT);
            // two lines to save horizontal space: "NAME PRICE" on top, time below
            char l1[40]; sprintf_s(l1, sizeof(l1), "%s %d", s.name.c_str(), (int)(s.price + 0.5f));
            short hh = (short)(fs / 2 + 1);
            textLJ((short)(R + 5), (short)(y - hh),     l1,             sc, fs, false);
            textLJ((short)(R + 5), (short)(y + hh + 1), s.time.c_str(), sc, fs, false);
        }
    }
}

// ---- expected-candle extras: E-HOD / E-LOD / E-C value tips ---------------
void DayModel::drawExpExtras(const DayModelState& state, short cx, const Settings& S)
{
    if (!state.expC.valid) return;
    int fs = S.font - 1; if (fs < 7) fs = 7;
    short step = (short)(fs + 3);
    COLOR ink = lerpColor(C_TXT, C_DARK, 0.28f);   // dimmer, matches the ghost
    char v[24];
    // E-HOD tip: value, then expected time + duration (when the model feed has them), stacked up
    if (S.hilo) {
        short hY = yOf(state, state.expC.h);
        sprintf_s(v, sizeof(v), "E-HOD %d", (int)(state.expC.h + 0.5f)); textC(cx, (short)(hY - step), v, ink, fs, false);
        if (state.hasEHodT) { textC(cx, (short)(hY - 2*step), state.ehodT.c_str(), ink, fs, false);
                        textC(cx, (short)(hY - 3*step), state.ehodD.c_str(), ink, fs, false); }
        short lY = yOf(state, state.expC.l);
        sprintf_s(v, sizeof(v), "E-LOD %d", (int)(state.expC.l + 0.5f)); textC(cx, (short)(lY + step), v, ink, fs, false);
        if (state.hasELodT) { textC(cx, (short)(lY + 2*step), state.elodT.c_str(), ink, fs, false);
                        textC(cx, (short)(lY + 3*step), state.elodD.c_str(), ink, fs, false); }
    }
    // expected MUD box inside the ghost body (mirrors the actual candle, dim ink)
    // (v0.13) label above the points, same as the actual MUD box.
    if (S.mud && state.hasEMud) {
        short oY = yOf(state, state.expC.o), cY = yOf(state, state.expC.c);
        short mid = (short)((oY + cY) / 2);
        char d[24]; sprintf_s(d, sizeof(d), "$%s", state.emudDol.c_str());
        textC(cx, (short)(mid - 2*step), "E-MUD",        ink, fs, true);
        textC(cx, (short)(mid - step),   state.emudP.c_str(),  ink, fs, true);
        textC(cx, mid,                   state.emudT.c_str(),  ink, fs, false);
        textC(cx, (short)(mid + step),   d,              ink, fs, false);
    }
}

// ---- render ---------------------------------------------------------------
void DayModel::render(DayModelState& state, const Settings& S)
{
    if (!state.expC.valid && !state.actC.valid) return;
    // (v0.14) STALE GUARD (see the long note below at the draw calls): a badly stale CSV means the
    // EXPECTED candle is frozen onto an old book. Decide it up-front so the panel bounds, background,
    // E-lines and the candle itself all agree — no oversized panel sized to a candle we won't draw.
    bool expStale = (staleAgeMin(state) >= 15.0);
    if (expStale && !state.actC.valid) return;   // draw() renders the badge exactly once after render()
    long n = getBarCount(); if (n < 1) return;
    state.lastBar = (int)n - 1;

    RCT pane; pane.getPaneRect(false);
    short paneL = pane.left, paneR = pane.right;

    short half = (short)(S.width / 2);
    short inner = (short)S.spacing;          // gap between the two candles in Pair layout
    int fs = S.font - 1; if (fs < 7) fs = 7;
    short step = (short)(fs + 3);
    // (v0.7) LEFT-ANNOTATION ALLOWANCE. The EXP candle's tips (E-HOD / E-MUD) are CENTRED on its column,
    // so on the left margin they spill past the pane's left edge and clip. Reserve room to their left and
    // extend the panel to cover them. ~half the widest tip ("E-MUD +63.2" / a 12h clock).
    short annoL = (short)(fs * 5 + 8);

    short expCx, actCx;
    if (S.layout == 1) {                     // Overlay: same column
        short cx = (S.side == 1) ? (short)(paneR - S.gap - half) : (short)(paneL + S.gap + annoL + half);
        expCx = cx; actCx = cx;
    } else {                                 // Pair: two columns; ACT nearest price
        if (S.side == 1) {                   // Right margin: ACT on the right
            actCx = (short)(paneR - S.gap - half);
            expCx = (short)(actCx - S.width - inner);
        } else {                             // Left margin (default): ACT on the right of the pair
            expCx = (short)(paneL + S.gap + annoL + half);
            actCx = (short)(expCx + S.width + inner);
        }
    }

    // panel bounds (shared by the background fill and the header)
    short xLc = expCx < actCx ? expCx : actCx;
    short xRc = expCx > actCx ? expCx : actCx;
    short xL = (short)(xLc - half - (S.side == 1 ? 6 : annoL));   // (v0.7) cover the EXP tips on the left margin
    short xR = (short)(xRc + half + (S.swept ? 84 : 8));    // room for the (now narrower) swept tags
    float hiP = -1e9f, loP = 1e9f;
    if (state.expC.valid && !expStale) { if (state.expC.h>hiP) hiP=state.expC.h; if (state.expC.l<loP) loP=state.expC.l; }   // (v0.14) skip frozen EXP
    if (state.actC.valid) { if (state.actC.h>hiP) hiP=state.actC.h; if (state.actC.l<loP) loP=state.actC.l; }
    short headH = (short)(S.header ? (step + 6) : 0);
    short yT = (short)(yOf(state, hiP) - 3*step - 10 - headH);
    short yB = (short)(yOf(state, loP) + 3*step + 10);

    // background panel (drawn FIRST so chart bars/labels don't bleed through)
    if (S.bg) {
        RCT bgr; bgr.set(xL, yT, xR, yB);
        bgr.draw(1, C_BORDER, C_PANEL, DRAW_OPAQUE, PAT_SOLID);
    }

    // day-of-week header (+ date), top-centre of the panel
    if (S.header) {
        std::string hdr = state.weekday.empty() ? "DAY" : state.weekday;
        if (!state.daydate.empty()) { hdr += " "; hdr += state.daydate; }   // (v0.7) "Fri 11 Sep"
        textC((short)((xL + xR) / 2), (short)(yT + step - 2), hdr.c_str(), C_TXT, S.font, true);
    }

    // (v0.14) STALE GUARD (expStale decided at the top of render) — the EXPECTED candle is the CSV DAYEXP
    // row; when the panel stops writing it freezes onto a book that no longer matches the chart, and the
    // operator sees a huge, mis-aligned candle "printing for no reason" (operator-reported 2026-09-15, file
    // cold since 23:58 the night before). Past 15 min — five missed 3-min writes, well beyond normal jitter
    // — we STOP drawing the expected candle and its annotations. The ACTUAL candle is measured live from
    // THIS chart's own bars (measureChartDay), so it stays; the STALE badge still explains why EXP is gone.

    // optional E-HOD / E-LOD reference lines across the pane
    if (S.elines && state.expC.valid && !expStale) {
        hline(yOf(state, state.expC.h), paneL, paneR, C_ELINE, P_DASH);
        hline(yOf(state, state.expC.l), paneL, paneR, C_ELINE, P_DASH);
    }

    // Expected behind, Actual on top (matters in Overlay).
    if (S.showexp && state.expC.valid && !expStale) { drawCandle(state, expCx, state.expC, true,  S); drawExpExtras(state, expCx, S); }
    if (S.showact && state.actC.valid) { drawCandle(state, actCx, state.actC, false, S); drawActExtras(state, actCx, S); }
}

// ---- draw() ---------------------------------------------------------------
int DayModel::draw(void)
{
    DayModelState* state = getState(true);
    if (!state) return RTX_FAIL;
    load(*state);
    std::vector<dml::Bar> bars;
    loadBars(bars, 20000); // one bounded SDK-array/localtime pass per repaint
    detectBarStamp(*state, bars);  // start- or end-stamped bars decide which bar opens the RTH day
    // The ACTUAL candle is measured straight from THIS chart's own RTH session bars. If the session
    // has not opened yet, fall back to shifting the CSV candle onto the current chart.
    if (!measureChartDay(*state, bars)) applyContractOffset(*state, bars);
    computeChartLevels(*state, bars);
    render(*state, state->cfg);
    drawStaleBadge(*state);
    return RTX_OK;
}

// ---- (v0.16) BAR STAMP CONVENTION — the one thing that decides which bar is the FIRST of the RTH day ----------
// Investor/RT can stamp an intraday bar with its START or its END time (a chart preference). On a 3-minute chart
// aligned to the 17:00 session open, the 08:27-08:30 bar is stamped 08:27 (start) or 08:30 (end). Treating an
// END-stamped 08:30 bar as RTH lets three PRE-OPEN minutes into the prior-day high: 2026-09-16 the plugin printed
// PDH 7690 (the 08:27 spike, 7690.25) while IRT's own pDHI read 7687.00 — operator: "regarding the levels swept,
// I don't think it matches IRT." Detect the convention from the chart itself: a Full-Session 17:00-16:00 chart has a
// bar stamped exactly 17:00:00 only when START-stamped, and one stamped exactly 16:00:00 only when END-stamped.
// Any other session start/stop works the same way (the first bar is stamped at the boundary under one convention
// and one period past it under the other). If neither boundary stamp is found, START is assumed (the old behaviour).
// (v0.17) the chart's bars as plain records for DayModelLogic.h (the last `maxBars` of them)
void DayModel::loadBars(std::vector<dml::Bar>& out, int maxBars)
{
    long n = getBarCount(); out.clear(); if (n < 1) return;
    RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI dt(barDateTime);
    int from = (int)n - maxBars; if (from < 0) from = 0;
    out.reserve((size_t)((int)n - from));
    for (int i = from; i < (int)n; i++) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        dml::Bar b; b.y = t.tm_year + 1900; b.m = t.tm_mon + 1; b.d = t.tm_mday;
        b.sod = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec; b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i];
        out.push_back(b);
    }
}
void DayModel::detectBarStamp(DayModelState& state, const std::vector<dml::Bar>& bars)
{
    state.endStamped = dml::endStamped(bars.empty() ? 0 : &bars[0], (int)bars.size());
}

// ---- (v0.11) CHART-NATIVE REFERENCE LEVELS -----------------------------------------------------------
// PDH/PDL (prior RTH day high/low) and ONH/ONL (overnight high/low) are pure price levels IRT already
// knows, so read them straight from the chart's own bars — exact on whatever contract is charted. We keep
// the panel's sweep STATUS/TIME (R/B/T) by matching on the level name; only the PRICE is replaced. The
// other swept levels (PWH/PWL/PFH/PFL) keep the panel value (open-anchored) until they're made native too.
void DayModel::computeChartLevels(DayModelState& state, const std::vector<dml::Bar>& bars)
{
    if (state.swepts.empty()) return;
    if (bars.size() < 2) return;
    dml::Levels L = dml::chartLevels(&bars[0], (int)bars.size(), state.endStamped);
    for (size_t i = 0; i < state.swepts.size(); i++) {
        const std::string& nm = state.swepts[i].name;
        if      (L.hp && nm == "PDH") state.swepts[i].price = L.pdh;
        else if (L.hp && nm == "PDL") state.swepts[i].price = L.pdl;
        else if (L.ho && nm == "ONH") state.swepts[i].price = L.onh;
        else if (L.ho && nm == "ONL") state.swepts[i].price = L.onl;
    }
}

// ---- (v0.10) MEASURE THE ACTUAL CANDLE FROM THE CHART ITSELF -----------------------------------------
// Scan the chart's own bars for TODAY's RTH session (08:30–15:00 CT — the day-model window) and build the
// actual candle O/H/L/C + HOD/LOD from them. That makes the actual candle exactly the ES session high/low
// the operator sees, regardless of which contract is charted. The EXPECTED ghost + swept levels (which
// come from the panel in cash space) are re-anchored by the OPEN spread (dOpen − the panel's cash open),
// a stable per-day anchor, so they read against the same chart. Returns false pre-open (no RTH bars yet),
// so draw() can fall back to the offset path.
bool DayModel::measureChartDay(DayModelState& state, const std::vector<dml::Bar>& bars)
{
    if (bars.empty()) return false;
    dml::DayMeasure M = dml::measureDay(&bars[0], (int)bars.size(), state.endStamped);
    if (!M.ok) return false;
    float dayOff = dml::dayOffset(M.o, state.actC.valid, state.actC.o);   // open-anchored spread (stable across the day)
    state.actC.o = M.o; state.actC.h = M.h; state.actC.l = M.l; state.actC.c = M.c; state.actC.valid = true;   // exact ES session
    if (state.hasHod) state.hodV = M.h;
    if (state.hasLod) state.lodV = M.l;
    if (state.expC.valid) { state.expC.o += dayOff; state.expC.h += dayOff; state.expC.l += dayOff; state.expC.c += dayOff; }
    for (size_t i = 0; i < state.swepts.size(); i++) state.swepts[i].price += dayOff;
    return true;
}

// ---- (v0.9) CONTRACT ALIGNMENT — the candle is priced in the panel's space (cash / SPY×10, SPOT is the
// anchor). If this chart is a different contract (EPZ26 December, ~+70 over cash), the candle draws off
// the bottom. Shift every price by (this chart's last close − SPOT) so the candle sits on the chart.
void DayModel::applyContractOffset(DayModelState& state, const std::vector<dml::Bar>& bars)
{
    if (!state.hasSpot || bars.empty()) return;
    float chartClose = bars.back().c, off = 0.0f;
    if (!dml::contractOffset(chartClose, state.spotPx, off)) return;   // (v0.17) clamp + "already aligned" in DayModelLogic.h
    if (state.expC.valid) { state.expC.o+=off; state.expC.h+=off; state.expC.l+=off; state.expC.c+=off; }
    if (state.actC.valid) { state.actC.o+=off; state.actC.h+=off; state.actC.l+=off; state.actC.c+=off; }
    if (state.hasHod) state.hodV+=off;
    if (state.hasLod) state.lodV+=off;
    for (size_t i = 0; i < state.swepts.size(); i++) state.swepts[i].price += off;
}

// ---- (v0.8) STALE badge — the panel stamps ASOF,<CT sec-of-day> each export; compare to the chart
// clock (assumed CT). Older than ~4 min (panel writes every ~3 min) ⇒ a frozen file drawing an old
// book. Handles the overnight wrap so a file left cold overnight reads hours, not a negative age.
// (v0.14) ONE age computation, shared by the badge and the expected-candle guard. Returns minutes since
// the panel's ASOF write in chart-local (CT) time, handling the overnight wrap so a file left cold
// overnight reads hours, not a negative age. <0 when ASOF is unknown (nothing to judge).
double DayModel::staleAgeMin(const DayModelState& state)
{
    if (state.asofSo < 0) return -1.0;
    RTDATE now = currentDate(); struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime(now, &tmv);
    double localSo = tmv.tm_hour * 3600.0 + tmv.tm_min * 60.0 + tmv.tm_sec;
    return dml::staleAge(state.asofSo, localSo);   // (v0.17) DayModelLogic.h
}

void DayModel::drawStaleBadge(const DayModelState& state)
{
    double ageMin = staleAgeMin(state);
    if (ageMin < 0) return;
    if (!dml::staleBadgeShown(ageMin)) return;
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
    DayModel *p = new DayModel();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("Day model candle (expected + actual), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.20");
    return p;
}
