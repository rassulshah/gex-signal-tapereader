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
 *
 *  Parameter indices are numbered EXPLICITLY (pc++), one per control, with NO
 *  setLabelParameter section headers -- a label row shifts IRT's parameter
 *  numbering out from under the value getters and silently scrambles settings.
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
    if (t < 0) t = 0; if (t > 1) t = 1;
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

// ---------------------------------------------------------------------------
class DayModel : public cppExtension {
public:
    DayModel();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    DayCandle expC, actC;
    // finished-candle data
    float hodV, lodV; std::string hodT, hodD, lodT, lodD; bool hasHod, hasLod;
    std::string mudP, mudT, mudDol; bool hasMud;
    // expected-candle extras (time/duration/MUD from the model feed)
    std::string ehodT, ehodD, elodT, elodD; bool hasEHodT, hasELodT;
    std::string emudP, emudT, emudDol; bool hasEMud;
    std::vector<SweptLvl> swepts;
    std::string weekday, daydate;
    double asofSo;              // (v0.8) ASOF write-time (CT sec-of-day) for the STALE badge; <0 = unknown
    float spotPx; bool hasSpot; // (v0.9) SPOT anchor for the contract offset

    Settings cfg;
    int lastBar;

    void load();
    double staleAgeMin();      // (v0.14) minutes since ASOF write (CT), overnight-wrap aware; <0 = unknown
    void drawStaleBadge();     // (v0.8) red badge if the CSV has gone cold
    void applyContractOffset();// (v0.9) fallback: shift the CSV candle onto this chart (pre-open)
    bool measureChartDay();    // (v0.10) ACTUAL candle straight from the chart's own RTH session bars
    void computeChartLevels(); // (v0.11) PDH/PDL + overnight H/L from the chart's own bars (exact)
    void detectBarStamp();   // (v0.16) start- vs end-stamped bars (sets g_endStamped)
    void readSettings(Settings& S);
    void render(const Settings& S);
    // helpers
    short yOf(float price);
    void vline(short x, short y1, short y2, COLOR col, PEN_STYLE ps);
    void hline(short y, short x1, short x2, COLOR col, PEN_STYLE ps);
    void rectOutline(short l, short t, short r, short b, COLOR col, PEN_STYLE ps);
    void rectFill(short l, short t, short r, short b, COLOR fill, COLOR outline);
    void textC(short cx, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold);
    void drawCandle(short cx, const DayCandle& d, bool ghost, const Settings& S);
    void drawActExtras(short cx, const Settings& S);
    void drawExpExtras(short cx, const Settings& S);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- constructor: seed safe defaults so the first draw is valid -----------
DayModel::DayModel() : cppExtension()
{
    expC.valid = false; actC.valid = false; lastBar = 0;
    hasHod = false; hasLod = false; hasMud = false;
    hasEHodT = false; hasELodT = false; hasEMud = false;
    cfg.side = 0;          // Left margin (per the settled §10.1 layout)
    cfg.layout = 0;        // Pair (side by side)
    cfg.width = 46;
    cfg.spacing = 24;      // gap between EXP and ACT (was hardcoded 8 -- too close)
    cfg.gap = 12;
    cfg.bg = false;   // (v0.10) OFF by default — the big panel was tinting the chart
    cfg.showexp = true;
    cfg.showact = true;
    cfg.cup = C_UP;
    cfg.cdn = C_DN;
    cfg.font = 10;
    cfg.labels = false;    // the dashed one is obviously EXP -- no EXP/ACT tags by default
    cfg.hilo = true;
    cfg.mud = true;
    cfg.swept = true;
    cfg.header = true;
    cfg.elines = false;    // E-HOD/E-LOD reference lines off by default
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
void DayModel::load()
{
    expC.valid = false; actC.valid = false;
    hasHod = false; hasLod = false; hasMud = false; swepts.clear(); weekday.clear(); daydate.clear();
    hasEHodT = false; hasELodT = false; hasEMud = false; asofSo = -1; hasSpot = false; spotPx = 0.0f;
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
            expC.o=(float)atof(t[1].c_str()); expC.h=(float)atof(t[2].c_str());
            expC.l=(float)atof(t[3].c_str()); expC.c=(float)atof(t[4].c_str()); expC.valid=true;
        } else if (t[0] == "DAYACT" && t.size() >= 5) {
            actC.o=(float)atof(t[1].c_str()); actC.h=(float)atof(t[2].c_str());
            actC.l=(float)atof(t[3].c_str()); actC.c=(float)atof(t[4].c_str()); actC.valid=true;
        } else if (t[0] == "DAYHOD" && t.size() >= 4) {
            hodV=(float)atof(t[1].c_str()); hodT=t[2]; hodD=t[3]; hasHod=true;
        } else if (t[0] == "DAYLOD" && t.size() >= 4) {
            lodV=(float)atof(t[1].c_str()); lodT=t[2]; lodD=t[3]; hasLod=true;
        } else if (t[0] == "DAYMUD" && t.size() >= 4) {
            mudP=t[1]; mudT=t[2]; mudDol=t[3]; hasMud=true;
        } else if (t[0] == "SWEPT" && t.size() >= 5) {
            SweptLvl s; s.name=t[1]; s.price=(float)atof(t[2].c_str()); s.time=t[3];
            s.state = t[4].empty() ? 'T' : t[4][0];
            swepts.push_back(s);
        } else if (t[0] == "DAYEHOD" && t.size() >= 4) {
            ehodT=t[2]; ehodD=t[3]; hasEHodT=true;
        } else if (t[0] == "DAYELOD" && t.size() >= 4) {
            elodT=t[2]; elodD=t[3]; hasELodT=true;
        } else if (t[0] == "DAYEMUD" && t.size() >= 4) {
            emudP=t[1]; emudT=t[2]; emudDol=t[3]; hasEMud=true;
        } else if (t[0] == "WEEKDAY" && t.size() >= 2) {
            weekday = t[1];
            if (t.size() >= 3) daydate = t[2];   // (v0.7) optional date, e.g. "11 Sep"
        } else if (t[0] == "ASOF" && t.size() >= 2) {
            asofSo = atof(t[1].c_str());
        } else if (t[0] == "SPOT" && t.size() >= 2) {
            spotPx = (float)atof(t[1].c_str()); hasSpot = true;
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
void DayModel::drawActExtras(short cx, const Settings& S)
{
    if (!actC.valid) return;
    short half = (short)(S.width / 2);
    short L = (short)(cx - half), R = (short)(cx + half);
    int fs = S.font - 1; if (fs < 7) fs = 7;
    short step = (short)(fs + 3);

    // HOD tip: value nearest the high tip, then time, then duration, stacked upward
    if (S.hilo && hasHod) {
        short hY = yOf(actC.h);
        char v[24]; sprintf_s(v, sizeof(v), "HOD %d", (int)(hodV + 0.5f));
        textC(cx, (short)(hY - step),     v,            C_TXT, fs, true);
        textC(cx, (short)(hY - 2*step),   hodT.c_str(), C_TXT, fs, false);
        textC(cx, (short)(hY - 3*step),   hodD.c_str(), C_TXT, fs, false);
    }
    // LOD tip: stacked downward below the low tip
    if (S.hilo && hasLod) {
        short lY = yOf(actC.l);
        char v[24]; sprintf_s(v, sizeof(v), "LOD %d", (int)(lodV + 0.5f));
        textC(cx, (short)(lY + step),     v,            C_TXT, fs, true);
        textC(cx, (short)(lY + 2*step),   lodT.c_str(), C_TXT, fs, false);
        textC(cx, (short)(lY + 3*step),   lodD.c_str(), C_TXT, fs, false);
    }
    // MUD box inside the body: label / points / time / dollar
    // (v0.13) operator: "MUD should be on another line above the number of points" — label and value split.
    if (S.mud && hasMud) {
        short oY = yOf(actC.o), cY = yOf(actC.c);
        short mid = (short)((oY + cY) / 2);
        char d[24]; sprintf_s(d, sizeof(d), "$%s", mudDol.c_str());
        textC(cx, (short)(mid - 2*step), "MUD",          C_WHT, fs, true);   // label on its own line, above the points
        textC(cx, (short)(mid - step),   mudP.c_str(),   C_WHT, fs, true);   // the number of points
        textC(cx, mid,                   mudT.c_str(),   C_WHT, fs, false);
        textC(cx, (short)(mid + step),   d,              C_WHT, fs, false);
    }
    // swept levels: a dashed tick across the candle + a tag to the RIGHT
    if (S.swept) {
        for (size_t i = 0; i < swepts.size(); i++) {
            const SweptLvl& s = swepts[i];
            short y = yOf(s.price);
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
void DayModel::drawExpExtras(short cx, const Settings& S)
{
    if (!expC.valid) return;
    int fs = S.font - 1; if (fs < 7) fs = 7;
    short step = (short)(fs + 3);
    COLOR ink = lerpColor(C_TXT, C_DARK, 0.28f);   // dimmer, matches the ghost
    char v[24];
    // E-HOD tip: value, then expected time + duration (when the model feed has them), stacked up
    if (S.hilo) {
        short hY = yOf(expC.h);
        sprintf_s(v, sizeof(v), "E-HOD %d", (int)(expC.h + 0.5f)); textC(cx, (short)(hY - step), v, ink, fs, false);
        if (hasEHodT) { textC(cx, (short)(hY - 2*step), ehodT.c_str(), ink, fs, false);
                        textC(cx, (short)(hY - 3*step), ehodD.c_str(), ink, fs, false); }
        short lY = yOf(expC.l);
        sprintf_s(v, sizeof(v), "E-LOD %d", (int)(expC.l + 0.5f)); textC(cx, (short)(lY + step), v, ink, fs, false);
        if (hasELodT) { textC(cx, (short)(lY + 2*step), elodT.c_str(), ink, fs, false);
                        textC(cx, (short)(lY + 3*step), elodD.c_str(), ink, fs, false); }
    }
    // expected MUD box inside the ghost body (mirrors the actual candle, dim ink)
    // (v0.13) label above the points, same as the actual MUD box.
    if (S.mud && hasEMud) {
        short oY = yOf(expC.o), cY = yOf(expC.c);
        short mid = (short)((oY + cY) / 2);
        char d[24]; sprintf_s(d, sizeof(d), "$%s", emudDol.c_str());
        textC(cx, (short)(mid - 2*step), "E-MUD",        ink, fs, true);
        textC(cx, (short)(mid - step),   emudP.c_str(),  ink, fs, true);
        textC(cx, mid,                   emudT.c_str(),  ink, fs, false);
        textC(cx, (short)(mid + step),   d,              ink, fs, false);
    }
}

// ---- render ---------------------------------------------------------------
void DayModel::render(const Settings& S)
{
    if (!expC.valid && !actC.valid) return;
    // (v0.14) STALE GUARD (see the long note below at the draw calls): a badly stale CSV means the
    // EXPECTED candle is frozen onto an old book. Decide it up-front so the panel bounds, background,
    // E-lines and the candle itself all agree — no oversized panel sized to a candle we won't draw.
    bool expStale = (staleAgeMin() >= 15.0);
    if (expStale && !actC.valid) { drawStaleBadge(); return; }   // nothing live to show; badge tells why
    long n = getBarCount(); if (n < 1) return;
    lastBar = (int)n - 1;

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
    if (expC.valid && !expStale) { if (expC.h>hiP) hiP=expC.h; if (expC.l<loP) loP=expC.l; }   // (v0.14) skip frozen EXP
    if (actC.valid) { if (actC.h>hiP) hiP=actC.h; if (actC.l<loP) loP=actC.l; }
    short headH = (short)(S.header ? (step + 6) : 0);
    short yT = (short)(yOf(hiP) - 3*step - 10 - headH);
    short yB = (short)(yOf(loP) + 3*step + 10);

    // background panel (drawn FIRST so chart bars/labels don't bleed through)
    if (S.bg) {
        RCT bgr; bgr.set(xL, yT, xR, yB);
        bgr.draw(1, C_BORDER, C_PANEL, DRAW_OPAQUE, PAT_SOLID);
    }

    // day-of-week header (+ date), top-centre of the panel
    if (S.header) {
        std::string hdr = weekday.empty() ? "DAY" : weekday;
        if (!daydate.empty()) { hdr += " "; hdr += daydate; }   // (v0.7) "Fri 11 Sep"
        textC((short)((xL + xR) / 2), (short)(yT + step - 2), hdr.c_str(), C_TXT, S.font, true);
    }

    // (v0.14) STALE GUARD (expStale decided at the top of render) — the EXPECTED candle is the CSV DAYEXP
    // row; when the panel stops writing it freezes onto a book that no longer matches the chart, and the
    // operator sees a huge, mis-aligned candle "printing for no reason" (operator-reported 2026-09-15, file
    // cold since 23:58 the night before). Past 15 min — five missed 3-min writes, well beyond normal jitter
    // — we STOP drawing the expected candle and its annotations. The ACTUAL candle is measured live from
    // THIS chart's own bars (measureChartDay), so it stays; the STALE badge still explains why EXP is gone.

    // optional E-HOD / E-LOD reference lines across the pane
    if (S.elines && expC.valid && !expStale) {
        hline(yOf(expC.h), paneL, paneR, C_ELINE, P_DASH);
        hline(yOf(expC.l), paneL, paneR, C_ELINE, P_DASH);
    }

    // Expected behind, Actual on top (matters in Overlay).
    if (S.showexp && expC.valid && !expStale) { drawCandle(expCx, expC, true,  S); drawExpExtras(expCx, S); }
    if (S.showact && actC.valid) { drawCandle(actCx, actC, false, S); drawActExtras(actCx, S); }
}

// ---- draw() ---------------------------------------------------------------
int DayModel::draw(void)
{
    load();
    detectBarStamp();   // (v0.16) start- or end-stamped bars decide which bar opens the RTH day
    // (v0.10) The ACTUAL candle is measured straight from THIS chart's own RTH session bars, so it IS
    // the ES session high/low by definition — no cash→contract guesswork. If the session hasn't opened
    // yet (no RTH bars), fall back to shifting the CSV candle onto the chart.
    if (!measureChartDay()) applyContractOffset();
    computeChartLevels();   // (v0.11) override PDH/PDL/ONH/ONL swept prices with exact chart values
    render(cfg);
    drawStaleBadge();   // (v0.8) warn if the CSV is cold, regardless of what render drew
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
static bool g_endStamped = false;
void DayModel::detectBarStamp()
{
    long n = getBarCount(); if (n < 2) { g_endStamped = false; return; }
    RTARRAYI dt(barDateTime);
    int seenStart = 0, seenEnd = 0;
    int from = (int)n - 1, to = (int)n - 2000; if (to < 0) to = 0;
    for (int i = from; i >= to; i--) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        int sod = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
        if (sod == 17 * 3600) seenStart++;          // a bar STARTING at the 17:00 session open
        if (sod == 16 * 3600) seenEnd++;            // a bar ENDING at the 16:00 session close
    }
    g_endStamped = (seenEnd > 0 && seenStart == 0);
}
// RTH membership under the detected convention: START-stamped = [08:30, 15:00), END-stamped = (08:30, 15:00].
static bool inRth(int sod)
{
    const int O = 8 * 3600 + 30 * 60, S = 15 * 3600;
    return g_endStamped ? (sod > O && sod <= S) : (sod >= O && sod < S);
}
// overnight membership (prior evening >= 17:00 through today's pre-open): START = [17:00 ..) & [.. 08:30); END = (17:00 ..] & (.. 08:30]
static bool inEvening(int sod){ const int E = 17 * 3600; return g_endStamped ? (sod > E) : (sod >= E); }
static bool inPreOpen(int sod){ const int O = 8 * 3600 + 30 * 60; return g_endStamped ? (sod <= O) : (sod < O); }

// ---- (v0.11) CHART-NATIVE REFERENCE LEVELS -----------------------------------------------------------
// PDH/PDL (prior RTH day high/low) and ONH/ONL (overnight high/low) are pure price levels IRT already
// knows, so read them straight from the chart's own bars — exact on whatever contract is charted. We keep
// the panel's sweep STATUS/TIME (R/B/T) by matching on the level name; only the PRICE is replaced. The
// other swept levels (PWH/PWL/PFH/PFL) keep the panel value (open-anchored) until they're made native too.
void DayModel::computeChartLevels()
{
    if (swepts.empty()) return;
    long n = getBarCount(); if (n < 2) return;
    RTARRAY  hi(barHigh), lo(barLow);
    RTARRAYI dt(barDateTime);
    struct tm lt; memset(&lt, 0, sizeof(lt)); getLocaltime((RTDATE)dt[(int)n - 1], &lt);
    int todayKey = (lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday;
    // (v0.16) the RTH / overnight windows follow the chart's bar-stamp convention (detectBarStamp, above)
    int priorKey = 0;
    for (int i = (int)n - 1; i >= 0; i--) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        int k = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
        int sod = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
        if (k < todayKey && inRth(sod)) { priorKey = k; break; }
    }
    float pdh = -1e9f, pdl = 1e9f, onh = -1e9f, onl = 1e9f; bool hp = false, ho = false;
    for (int i = 0; i < (int)n; i++) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        int k = (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
        int sod = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
        if (priorKey && k == priorKey && inRth(sod)) {
            if (hi[i] > pdh) pdh = hi[i]; if (lo[i] < pdl) pdl = lo[i]; hp = true;
        }
        // overnight into today: prior evening (17:00 on) through today's pre-open (to 08:30)
        if ((priorKey && k == priorKey && inEvening(sod)) || (k == todayKey && inPreOpen(sod))) {
            if (hi[i] > onh) onh = hi[i]; if (lo[i] < onl) onl = lo[i]; ho = true;
        }
    }
    for (size_t i = 0; i < swepts.size(); i++) {
        const std::string& nm = swepts[i].name;
        if      (hp && nm == "PDH") swepts[i].price = pdh;
        else if (hp && nm == "PDL") swepts[i].price = pdl;
        else if (ho && nm == "ONH") swepts[i].price = onh;
        else if (ho && nm == "ONL") swepts[i].price = onl;
    }
}

// ---- (v0.10) MEASURE THE ACTUAL CANDLE FROM THE CHART ITSELF -----------------------------------------
// Scan the chart's own bars for TODAY's RTH session (08:30–15:00 CT — the day-model window) and build the
// actual candle O/H/L/C + HOD/LOD from them. That makes the actual candle exactly the ES session high/low
// the operator sees, regardless of which contract is charted. The EXPECTED ghost + swept levels (which
// come from the panel in cash space) are re-anchored by the OPEN spread (dOpen − the panel's cash open),
// a stable per-day anchor, so they read against the same chart. Returns false pre-open (no RTH bars yet),
// so draw() can fall back to the offset path.
bool DayModel::measureChartDay()
{
    long n = getBarCount(); if (n < 2) return false;
    RTARRAY  op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYI dt(barDateTime);
    // (v0.15) THE RTH DAY, ALWAYS — not IRT's rolling session. The old PRIMARY used getStartStop(getDaySessionNumber()),
    // i.e. whatever session the chart is in RIGHT NOW. On a "Full Session 17:00-16:00" futures chart that is the
    // overnight+RTH block, and after the close it rolls onto the NEW evening session — so at 8 PM the "actual" candle
    // was measuring the evening session's low (7652) while the DAY STATS labels still said the RTH low was 7643.50
    // @ 9:54a (operator: "the actual candle low and the price chart low are different"). The day model IS the RTH
    // day (08:30–15:00 CT), so measure exactly that window of the most recent RTH day: today's if it has bars, else
    // step back (over a weekend too) so the completed day keeps showing overnight until the next open.
    struct tm lt; memset(&lt, 0, sizeof(lt)); getLocaltime((RTDATE)dt[(int)n - 1], &lt);
    float dOpen = 0, dHi = -1e9f, dLo = 1e9f, dClose = 0; bool have = false;
    for (int back = 0; back <= 4 && !have; back++) {
        // the calendar day to measure: the last bar's local date, minus `back` days
        struct tm want = lt; want.tm_mday -= back; want.tm_hour = 12; want.tm_min = 0; want.tm_sec = 0; want.tm_isdst = -1;
        time_t wt = mktime(&want); struct tm wn; memset(&wn, 0, sizeof(wn)); localtime_s(&wn, &wt);
        int wY = wn.tm_year, wM = wn.tm_mon, wD = wn.tm_mday;
        dHi = -1e9f; dLo = 1e9f; have = false;
        for (int i = 0; i < (int)n; i++) {
            struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime((RTDATE)dt[i], &tmv);
            if (tmv.tm_year != wY || tmv.tm_mon != wM || tmv.tm_mday != wD) continue;
            int sod = tmv.tm_hour * 3600 + tmv.tm_min * 60 + tmv.tm_sec;
            if (!inRth(sod)) continue;   // (v0.16) stamp-aware RTH window
            if (!have) { dOpen = op[i]; have = true; }
            if (hi[i] > dHi) dHi = hi[i];
            if (lo[i] < dLo) dLo = lo[i];
            dClose = cl[i];
        }
    }
    if (!have || !(dHi > dLo)) return false;
    float csvActO = actC.valid ? actC.o : dOpen;
    float dayOff  = dOpen - csvActO;            // open-anchored spread (stable across the day)
    actC.o = dOpen; actC.h = dHi; actC.l = dLo; actC.c = dClose; actC.valid = true;   // exact ES session
    if (hasHod) hodV = dHi;
    if (hasLod) lodV = dLo;
    if (expC.valid) { expC.o += dayOff; expC.h += dayOff; expC.l += dayOff; expC.c += dayOff; }
    for (size_t i = 0; i < swepts.size(); i++) swepts[i].price += dayOff;
    return true;
}

// ---- (v0.9) CONTRACT ALIGNMENT — the candle is priced in the panel's space (cash / SPY×10, SPOT is the
// anchor). If this chart is a different contract (EPZ26 December, ~+70 over cash), the candle draws off
// the bottom. Shift every price by (this chart's last close − SPOT) so the candle sits on the chart.
void DayModel::applyContractOffset()
{
    if (!hasSpot) return;
    long n = getBarCount(); if (n < 1) return;
    RTARRAY close(barClose);
    float chartClose = close[(int)n - 1];
    if (!(chartClose > 0)) return;
    float off = chartClose - spotPx;
    if (off < -300.0f || off > 300.0f) return;   // implausible → leave as-is
    if (off > -0.01f && off < 0.01f) return;      // already aligned
    if (expC.valid) { expC.o+=off; expC.h+=off; expC.l+=off; expC.c+=off; }
    if (actC.valid) { actC.o+=off; actC.h+=off; actC.l+=off; actC.c+=off; }
    if (hasHod) hodV+=off;
    if (hasLod) lodV+=off;
    for (size_t i = 0; i < swepts.size(); i++) swepts[i].price += off;
}

// ---- (v0.8) STALE badge — the panel stamps ASOF,<CT sec-of-day> each export; compare to the chart
// clock (assumed CT). Older than ~4 min (panel writes every ~3 min) ⇒ a frozen file drawing an old
// book. Handles the overnight wrap so a file left cold overnight reads hours, not a negative age.
// (v0.14) ONE age computation, shared by the badge and the expected-candle guard. Returns minutes since
// the panel's ASOF write in chart-local (CT) time, handling the overnight wrap so a file left cold
// overnight reads hours, not a negative age. <0 when ASOF is unknown (nothing to judge).
double DayModel::staleAgeMin()
{
    if (asofSo < 0) return -1.0;
    RTDATE now = currentDate(); struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime(now, &tmv);
    double localSo = tmv.tm_hour * 3600.0 + tmv.tm_min * 60.0 + tmv.tm_sec;
    return (asofSo > localSo + 300.0) ? ((86400.0 - asofSo) + localSo) / 60.0 : (localSo - asofSo) / 60.0;
}

void DayModel::drawStaleBadge()
{
    double ageMin = staleAgeMin();
    if (ageMin < 0) return;
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
    DayModel *p = new DayModel();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("Day model candle (expected + actual), reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.16");
    return p;
}
