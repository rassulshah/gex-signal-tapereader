/********************************************************************************
 *  GammaProfile.cpp  —  Investor/RT RTX extension  (gamma node profile)
 *
 *  Per-strike gamma histogram + level rail, drawn on the instrument price axis
 *  (INSTRUMENT_SCALE). Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv
 *  (or GammaProfile-SPY.csv when Book = SPY).
 *
 *  CSV: STRIKE,<price>,<pctKing -100..100>,<rank>,<isKing 0|1>[,<type>]
 *       KING,<p>  CW,<p>  PW,<p>  FLIP,<p>  EMH,<p>  EML,<p>  SPOT,<p>
 *       SPYKING,<p>   BOOK,<name>
 *
 *  Full settings panel (see setup()). Build: x64 Release, link irtsdkV143-x64.lib.
 *
 *  Parameter indices are captured dynamically via getParameterCount() into PX
 *  (below), so inserting section-header labels or new controls can never shift a
 *  control's index out from under readSettings().
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

// ---- default palette (matches the Skylit tape) ----------------------------
static const COLOR D_POS  = 0x00E3C341;  // +gamma  (yellow/gold)
static const COLOR D_NEG  = 0x00C43BAF;  // -gamma  (magenta)
static const COLOR D_MID  = 0x00179E5C;  // gamma ~ 0 (teal)
static const COLOR D_KING = 0x00FFF0A6;  // King (when "Distinct" chosen)
static const COLOR C_TXT  = 0x00DFE7F0;  // neutral label ink
static const COLOR C_DARK = 0x00101418;  // dark ink / bubble
static const COLOR C_WHT  = 0x00FFFFFF;
static const COLOR C_GREY = 0x00566472;  // greyed (sub-threshold) node
static const COLOR C_PINK = 0x00FF5DB0;  // walls
static const COLOR C_FLIPC= 0x00E6EDF5;  // flip
static const COLOR C_CYAN = 0x004FD0E0;  // EM band
static const COLOR C_SPOT = 0x005B6B7E;  // spot marker
static const COLOR C_SPYK = 0x0069D0A0;  // SPY King line (secondary book)

static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}
static float luma(COLOR c){ return 0.2126f*((c>>16)&0xFF) + 0.7152f*((c>>8)&0xFF) + 0.0722f*(c&0xFF); }
static COLOR inkOn(COLOR c){ return luma(c) > 140.0f ? C_DARK : C_WHT; }

// ---- parameter indices, filled in setup() (single-instance -> file static) --
struct PIdx {
    int book, width, side, thick, detach, round;
    int filter, thresh, below, scale;
    int cpos, cneg, cmid, kingcol, amp, trans;
    int showpct, pctpos, hideu, rank, rankpos, rankscope, type, kinglabel, font;
    int kline, cw, pw, flip, em, lstyle, lpos, extk, topnodes, topstyle, spyking;
    int header, spot;
};
static PIdx PX;

struct Settings {
    int book, width, side, thick, filter, thresh, below, scale, kingcol;
    int pctpos, hideu, rankpos, rankscope, font, lstyle, lpos, topstyle;
    bool detach, round, amp, trans, showpct, rank, type;
    bool kline, cw, pw, flip, em, extk, topnodes, spyking, header, spot;
    COLOR cpos, cneg, cmid;
    char kinglabel[24];
};

struct GStrike { float price; float pct; int rank; bool king; std::string type; };

// ---------------------------------------------------------------------------
class GammaProfile : public cppExtension {
public:
    GammaProfile();
    // Read settings ONLY from the parameter callbacks (dialog controls are valid there);
    // the constructor seeds safe defaults so the closed-dialog state renders correctly.
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    std::vector<GStrike> strikes;
    float lvl[6]; bool has[6];   // KING,CW,PW,FLIP,EMH,EML
    float spotPx; bool hasSpot;
    float spyKingPx; bool hasSpyKing;
    std::string book;
    Settings cfg;                // cached settings (populated in parms callbacks, used in draw)

    void load();
    void readSettings(Settings& S);
    void render(const Settings& S);
    void drawBar(short l, short t, short r, short b, COLOR col, bool rounded, bool trans);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX,  short y, const char* s, COLOR col, int sz, bool bold);
    void textC (short cx,     short y, const char* s, COLOR col, int sz, bool bold);
    void hlinePx(short y, short lx, short rx, COLOR col, PEN_STYLE ps);
    void drawLevel(int lastBar, short lx, short rx, int idx, COLOR col, const char* label,
                   bool extend, const Settings& S);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- constructor: cache safe defaults so the first draw (before calc) is valid
GammaProfile::GammaProfile() : cppExtension()
{
    cfg.book=0; cfg.width=130; cfg.side=0; cfg.detach=true; cfg.thick=0; cfg.round=true;
    cfg.filter=1; cfg.thresh=20; cfg.below=0; cfg.scale=0;
    cfg.cpos=D_POS; cfg.cneg=D_NEG; cfg.cmid=D_MID; cfg.kingcol=0; cfg.amp=false; cfg.trans=false;
    cfg.showpct=true; cfg.pctpos=0; cfg.hideu=5; cfg.rank=true; cfg.rankpos=0; cfg.rankscope=0;
    cfg.type=true; cfg.font=12;
    strncpy(cfg.kinglabel, "KING", sizeof(cfg.kinglabel)); cfg.kinglabel[sizeof(cfg.kinglabel)-1]=0;
    cfg.kline=true; cfg.cw=true; cfg.pw=true; cfg.flip=true; cfg.em=true;
    cfg.lstyle=0; cfg.lpos=0; cfg.extk=false; cfg.topnodes=false; cfg.topstyle=1; cfg.spyking=false;
    cfg.header=true; cfg.spot=true;
}

// ---- parameter callbacks: dialog controls are valid here, so read + cache.
// parmsLoad is guarded: only accept the read if a control reads a plausible value,
// otherwise keep the constructor defaults (so the profile always renders).
int GammaProfile::parmsLoad(void)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) readSettings(cfg);
    return RTX_OK;
}
int GammaProfile::parmsApply(void)            { readSettings(cfg); return RTX_OK; }
int GammaProfile::parmsUpdt(unsigned int)     { readSettings(cfg); return RTX_OK; }

// ---- parameter panel ------------------------------------------------------
// Each control's index is captured from getParameterCount() the instant before
// it is declared, so section-header labels (which DO consume an index) never
// shift a control's number.
int cppExtension::setup(void)
{
    // Parameter-layout version. IRT stores an instance's parameter values by
    // position; bumping this marks the layout as changed. BUMP whenever
    // parameters are reordered/inserted. (New params are APPENDED at the end
    // from here on, so existing instances keep mapping and this stays put.)
    setParameterVersion(4);
    setParameterDialogHeight(30);   // shorter: controls are paired two per row

    // Controls are laid out two per row via kParmAppendSameLine to use the
    // dialog's width (IRT fixes that width for its standard footer) and cut the
    // height. Each section header carries a one-line description of what it does.
    const short SL = kParmAppendSameLine;

    setLabelParameter("\x97 PROFILE   data book, size & placement of the strip");
    PX.book   = getParameterCount(); setListParameter   ("Book", 0, "Auto;SPX;SPY");
    PX.width  = getParameterCount(); setIntegerParameter("Width px", 130, 0, SL);
    PX.side   = getParameterCount(); setListParameter   ("Side", 0, "Right;Left");
    PX.thick  = getParameterCount(); setListParameter   ("Thickness", 0, "Auto;Thin;Medium;Thick", 0, SL);
    PX.detach = getParameterCount(); setBoolParameter   ("Detach from bars", true);
    PX.round  = getParameterCount(); setBoolParameter   ("Rounded ends", true, SL);

    setLabelParameter("\x97 NODES   which strikes show as bars, and how they scale");
    PX.filter = getParameterCount(); setListParameter   ("Show", 1, "Top 3;Top 5;Top 8;Top 10;>= Threshold;All");
    PX.thresh = getParameterCount(); setIntegerParameter("Threshold %", 20, 0, SL);
    PX.below  = getParameterCount(); setListParameter   ("Sub-threshold", 0, "Grey out;Hide");
    PX.scale  = getParameterCount(); setListParameter   ("Scale to", 0, "King=100%;Visible max", 0, SL);

    setLabelParameter("\x97 COLOR   gamma polarity colours for the bars");
    PX.cpos   = getParameterCount(); setColorParameter  ("+Gamma", D_POS);
    PX.cneg   = getParameterCount(); setColorParameter  ("-Gamma", D_NEG, 0, SL);
    PX.cmid   = getParameterCount(); setColorParameter  ("Midpoint", D_MID);
    PX.kingcol= getParameterCount(); setListParameter   ("King colour", 0, "Polarity;Distinct", 0, SL);
    PX.amp    = getParameterCount(); setBoolParameter   ("Amplify polarity", false);
    PX.trans  = getParameterCount(); setBoolParameter   ("Translucent bars", false, SL);

    setLabelParameter("\x97 LABELS   the % / node-name / rank text on each bar");
    PX.showpct= getParameterCount(); setBoolParameter   ("Show %", true);
    PX.pctpos = getParameterCount(); setListParameter   ("% at", 0, "Outside;Inside", 0, SL);
    PX.hideu  = getParameterCount(); setIntegerParameter("Hide % under", 5);
    PX.font   = getParameterCount(); setIntegerParameter("Font size (pt)", 12, 0, SL);
    PX.rank   = getParameterCount(); setBoolParameter   ("Rank badge", true);
    PX.type   = getParameterCount(); setBoolParameter   ("Node name inside", true, SL);
    PX.rankpos= getParameterCount(); setListParameter   ("Rank at", 0, "Inside;Outside");
    PX.rankscope=getParameterCount();setListParameter   ("Rank for", 0, "Top 5;Top 3", 0, SL);
    PX.kinglabel=getParameterCount();setListParameter   ("King name", 0, "KING;GPoc;GPOC;GEX;POC;GAMMA");

    setLabelParameter("\x97 LEVELS   horizontal price lines across the chart");
    PX.kline  = getParameterCount(); setBoolParameter   ("King", true);
    PX.cw     = getParameterCount(); setBoolParameter   ("Call Wall", true, SL);
    PX.pw     = getParameterCount(); setBoolParameter   ("Put Wall", true, SL);
    PX.flip   = getParameterCount(); setBoolParameter   ("Flip", true);
    PX.em     = getParameterCount(); setBoolParameter   ("EM H/L", true, SL);
    PX.extk   = getParameterCount(); setBoolParameter   ("Extend King", false, SL);
    PX.lstyle = getParameterCount(); setListParameter   ("Line style", 0, "Solid;Dot;Dash");
    PX.lpos   = getParameterCount(); setListParameter   ("Label at", 0, "Left;Center;Right", 0, SL);
    PX.topnodes=getParameterCount(); setBoolParameter   ("Top-node lines", false);
    PX.topstyle=getParameterCount(); setListParameter   ("Line", 1, "Solid;Dot;Dash", 0, SL);
    PX.spyking= getParameterCount(); setBoolParameter   ("SPY King line", false);

    setLabelParameter("\x97 CONTEXT   title text & the current-price line");
    PX.header = getParameterCount(); setBoolParameter   ("Title header (top-left text)", true);
    PX.spot   = getParameterCount(); setBoolParameter   ("Spot price line (dotted)", true);
    return RTX_OK;
}

// ---- read settings --------------------------------------------------------
void GammaProfile::readSettings(Settings& S)
{
    S.book  = getListIndex(PX.book);
    S.width = getIntegerValue(PX.width); if (S.width < 40) S.width = 40; if (S.width > 400) S.width = 400;
    S.side  = getListIndex(PX.side);
    S.thick = getListIndex(PX.thick);
    S.detach= isBoxChecked(PX.detach) != 0;
    S.round = isBoxChecked(PX.round) != 0;
    S.filter= getListIndex(PX.filter);
    S.thresh= getIntegerValue(PX.thresh); if (S.thresh < 0) S.thresh = 0;
    S.below = getListIndex(PX.below);
    S.scale = getListIndex(PX.scale);
    COLOR cp=(COLOR)(getIntegerValue(PX.cpos)&0xFFFFFF); S.cpos = cp ? cp : D_POS;
    COLOR cn=(COLOR)(getIntegerValue(PX.cneg)&0xFFFFFF); S.cneg = cn ? cn : D_NEG;
    COLOR cm=(COLOR)(getIntegerValue(PX.cmid)&0xFFFFFF); S.cmid = cm ? cm : D_MID;
    S.kingcol  = getListIndex(PX.kingcol);
    S.amp      = isBoxChecked(PX.amp) != 0;
    S.trans    = isBoxChecked(PX.trans) != 0;
    S.showpct  = isBoxChecked(PX.showpct) != 0;
    S.pctpos   = getListIndex(PX.pctpos);
    S.hideu    = getIntegerValue(PX.hideu); if (S.hideu < 0) S.hideu = 0;
    S.rank     = isBoxChecked(PX.rank) != 0;
    S.rankpos  = getListIndex(PX.rankpos);
    S.rankscope= getListIndex(PX.rankscope);   // 0=Top5, 1=Top3
    S.type     = isBoxChecked(PX.type) != 0;
    { char kl[24]=""; getListSelection(PX.kinglabel, kl, sizeof(kl));
      if (!kl[0]) strncpy(kl, "KING", sizeof(kl));
      strncpy(S.kinglabel, kl, sizeof(S.kinglabel)); S.kinglabel[sizeof(S.kinglabel)-1]=0; }
    S.font     = getIntegerValue(PX.font); if (S.font < 7) S.font = 7; if (S.font > 40) S.font = 40;
    S.kline = isBoxChecked(PX.kline) != 0;
    S.cw    = isBoxChecked(PX.cw) != 0;
    S.pw    = isBoxChecked(PX.pw) != 0;
    S.flip  = isBoxChecked(PX.flip) != 0;
    S.em    = isBoxChecked(PX.em) != 0;
    S.lstyle= getListIndex(PX.lstyle);
    S.lpos  = getListIndex(PX.lpos);
    S.extk  = isBoxChecked(PX.extk) != 0;
    S.topnodes = isBoxChecked(PX.topnodes) != 0;
    S.topstyle = getListIndex(PX.topstyle);
    S.spyking  = isBoxChecked(PX.spyking) != 0;
    S.header= isBoxChecked(PX.header) != 0;
    S.spot  = isBoxChecked(PX.spot) != 0;
}

// ---- data load ------------------------------------------------------------
void GammaProfile::load()
{
    for (int i = 0; i < 6; i++) { has[i] = false; lvl[i] = 0.0f; }
    hasSpot = false; spotPx = 0.0f; hasSpyKing = false; spyKingPx = 0.0f; book = "SPX";
    const char* up = getenv("USERPROFILE");
    if (!up) { strikes.clear(); return; }
    // Book selector: Auto(0) and SPX(1) read GammaProfile.csv; SPY(2) reads GammaProfile-SPY.csv.
    // (Auto currently defaults to the SPX book; true symbol-driven auto arrives with the live feed.)
    const char* fname = (cfg.book == 2) ? "GammaProfile-SPY.csv" : "GammaProfile.csv";
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" + fname;
    std::ifstream f(path.c_str());
    if (!f.is_open()) { strikes.clear(); return; }

    std::vector<GStrike> tmp;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t; std::stringstream ss(line); std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.empty()) continue;
        if (t[0] == "STRIKE" && t.size() >= 5) {
            GStrike s;
            s.price = (float)atof(t[1].c_str());
            s.pct   = (float)atof(t[2].c_str());
            s.rank  = atoi(t[3].c_str());
            s.king  = (atoi(t[4].c_str()) != 0);
            s.type  = (t.size() >= 6) ? t[5] : (s.king ? std::string("KING") : std::string());
            tmp.push_back(s);
        } else if (t.size() >= 2) {
            float v = (float)atof(t[1].c_str());
            if      (t[0] == "KING")    { lvl[0]=v; has[0]=true; }
            else if (t[0] == "CW")      { lvl[1]=v; has[1]=true; }
            else if (t[0] == "PW")      { lvl[2]=v; has[2]=true; }
            else if (t[0] == "FLIP")    { lvl[3]=v; has[3]=true; }
            else if (t[0] == "EMH")     { lvl[4]=v; has[4]=true; }
            else if (t[0] == "EML")     { lvl[5]=v; has[5]=true; }
            else if (t[0] == "SPOT")    { spotPx=v; hasSpot=true; }
            else if (t[0] == "SPYKING") { spyKingPx=v; hasSpyKing=true; }
            else if (t[0] == "BOOK" && t.size()>=2) { book=t[1]; }
        }
    }
    strikes.swap(tmp);
}

// ---- small drawing helpers ------------------------------------------------
void GammaProfile::drawBar(short l, short t, short r, short b, COLOR col, bool rounded, bool trans)
{
    RCT rc; rc.set(l, t, r, b);
    if (rounded) {
        setPen(col, 1, P_SOLID);
        CBRUSH br(col, PAT_SOLID); br.set();
        rc.drawRounded(6, 6);
    } else {
        rc.draw(0, col, col, trans ? DRAW_TRANSLUCENT : DRAW_OPAQUE, PAT_SOLID);
    }
}
void GammaProfile::textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    RCT rc; rc.set((short)(rightX - 260), (short)(y - sz), rightX, (short)(y + sz));
    rc.drawText(s, false, true);      // not centered, right-justified
}
void GammaProfile::textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    RCT rc; rc.set(leftX, (short)(y - sz), (short)(leftX + 260), (short)(y + sz));
    rc.drawText(s, false, false);     // left-justified
}
// Centered on (cx, y) both axes, using measured metrics.
void GammaProfile::textC(short cx, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    int lead=0, asc=0, desc=0; getFontMetrics(&lead, &asc, &desc);
    short tw = (short)getTextWidth(s, -1);
    setTextColor(col);
    PNT tp; tp.h = (short)(cx - tw/2); tp.v = (short)(y + (asc - desc)/2); tp.drawText(s);
}
// Horizontal line in pixel space (lets a level reach into the right margin).
void GammaProfile::hlinePx(short y, short lx, short rx, COLOR col, PEN_STYLE ps)
{
    setPen(col, 1, ps);
    PNT a; a.set(0, 0.0f); a.h = lx; a.v = y; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = rx; b.v = y; b.drawLineTo();
}

// ---- level rail -----------------------------------------------------------
void GammaProfile::drawLevel(int lastBar, short lx, short rx, int idx, COLOR col,
                             const char* label, bool extend, const Settings& S)
{
    if (!has[idx]) return;
    PEN_STYLE ps = S.lstyle==1 ? P_DOT : (S.lstyle==2 ? P_DASH : P_SOLID);
    setPen(col, 1, ps);
    PNT a; a.set(0, lvl[idx]);        a.setDrawPosition();
    PNT b; b.set(lastBar, lvl[idx]);
    if (extend) b.h = rx;             // stretch into the right margin
    b.drawLineTo();
    if (!label || !label[0]) return;   // line only (no label — used for KING, which the node labels)
    PNT probe; probe.set(lastBar, lvl[idx]); short y = probe.v;
    if (S.lpos == 0)      textLJ((short)(lx + 4),            (short)(y - S.font - 2), label, col, S.font, false); // left (default)
    else if (S.lpos == 1) textLJ((short)((lx + rx)/2 - 20),  (short)(y - S.font - 2), label, col, S.font, false); // center
    else                  textRJ((short)(rx - 4),            (short)(y - S.font - 2), label, col, S.font, false); // right
}

// ---- render ---------------------------------------------------------------
void GammaProfile::render(const Settings& S)
{
    if (strikes.empty()) return;
    long n = getBarCount(); if (n < 2) return;
    int lastBar = (int)n - 1;

    RCT pane; pane.getPaneRect(false);
    short paneL = pane.left, paneR = pane.right;

    // right edge of the last price bar (candles end here)
    PNT lb; lb.set(lastBar, strikes[0].price);
    short lastX = (short)(lb.h + getPixelsPerBar()/2);

    // effective width / anchor.
    // WIDTH IS FIXED at S.width so bar length NEVER rescales when the chart is
    // zoomed or scrolled. The old code tied width to the gap between the last
    // candle and the pane edge (which moves as you scroll) -> bars pulsed.
    short w = (short)S.width;
    short anchor;                 // base of bars
    int sgn;                      // +1 grows right, -1 grows left
    if (S.side == 1) {            // Left margin: anchor at pane left, grow right
        anchor = (short)(paneL + 2);
        sgn = +1;
    } else {                      // Right margin (default): anchor at pane right, grow left
        sgn = -1;
        if (S.detach) {
            anchor = paneR;                       // fixed strip pinned to the right margin
        } else {
            short a = (short)(lastX + w + 6);      // hug the candles (moves with them, fixed width)
            anchor = a < paneR ? a : paneR;
        }
    }

    // bar thickness
    short barH = 6;
    if (S.thick == 1) barH = 6; else if (S.thick == 2) barH = 12; else if (S.thick == 3) barH = 20;
    else { // Auto from spacing
        if (strikes.size() >= 2) {
            PNT a; a.set(lastBar, strikes[0].price);
            PNT b; b.set(lastBar, strikes[1].price);
            short d = (short)std::abs((int)b.v - (int)a.v);
            barH = d > 4 ? (short)(d * 0.78f) : 6;
        }
    }
    if (barH < 3) barH = 3; if (barH > 40) barH = 40;

    // ALL text (%, node name, rank, levels, header) uses ONE size: the user's
    // "Font size" setting (default 12pt). Bar thickness no longer changes text
    // size -- predictable, and the user scales everything with one control.

    // scale reference
    float maxAbs = 100.0f;
    if (S.scale == 1) { maxAbs = 1.0f; for (size_t i=0;i<strikes.size();i++){ float a=std::fabs(strikes[i].pct); if(a>maxAbs)maxAbs=a; } }

    // header
    if (S.header) {
        char h[96];
        int kingStrike = 0; for (size_t i=0;i<strikes.size();i++) if (strikes[i].king) kingStrike=(int)(strikes[i].price+0.5f);
        sprintf_s(h, sizeof(h), "%s gamma  King %d", book.c_str(), kingStrike);
        textLJ((short)(paneL + 6), (short)(pane.top + S.font + 2), h, C_TXT, S.font, true);
    }

    // spot marker
    if (S.spot && hasSpot) {
        PNT sp; sp.set(lastBar, spotPx);
        setPen(C_SPOT, 1, P_DOT);
        PNT a; a.set(0, spotPx); a.setDrawPosition();
        PNT b; b.set(lastBar, spotPx); b.drawLineTo();
    }

    PEN_STYLE tps = S.topstyle==1 ? P_DOT : (S.topstyle==2 ? P_DASH : P_SOLID);

    // bars
    for (size_t i = 0; i < strikes.size(); i++) {
        GStrike& s = strikes[i];
        float ab = std::fabs(s.pct);

        int topN = (S.filter==0)?3 : (S.filter==1)?5 : (S.filter==2)?8 : (S.filter==3)?10 : 0;
        bool primary;
        if      (S.filter <= 3) primary = (s.rank >= 1 && s.rank <= topN);
        else if (S.filter == 4) primary = (ab >= (float)S.thresh);
        else                    primary = true;
        if (!primary && S.below == 1) continue;   // hide

        PNT p; p.set(lastBar, s.price);
        short len = (short)(ab / maxAbs * w); if (len < 3) len = 3;
        short tip = (short)(anchor + sgn * len);

        // color
        float t = ab / 100.0f; if (S.amp) t = t <= 0 ? 0 : (float)std::sqrt(t);
        COLOR col = (s.pct >= 0) ? lerpColor(S.cmid, S.cpos, t) : lerpColor(S.cmid, S.cneg, t);
        if (s.king && S.kingcol == 1) col = D_KING;
        if (!primary) col = C_GREY;

        // top-node line: a horizontal rail at each primary node, in the node's colour
        if (S.topnodes && primary) hlinePx(p.v, paneL, paneR, col, tps);

        short top = (short)(p.v - barH/2), bot = (short)(p.v + barH/2);
        short bl = sgn < 0 ? tip : anchor, br = sgn < 0 ? anchor : tip;
        drawBar(bl, top, br, bot, col, S.round, S.trans);

        // node TYPE centered inside the bar (KING label overridable)
        const char* tlabel = s.king ? S.kinglabel : s.type.c_str();
        if (S.type && tlabel && tlabel[0] && len > (short)(S.font * 2)) {
            COLOR ic = inkOn(col);
            short cxbar = (short)((anchor + tip) / 2);   // horizontal center of the bar
            textC(cxbar, p.v, tlabel, ic, S.font, true);
        }

        // RANK bubble
        bool inScope = (S.rankscope == 1) ? (s.rank>=1 && s.rank<=3) : (s.rank>=1 && s.rank<=5);
        if (S.rank && inScope) {
            char rk[8]; sprintf_s(rk, sizeof(rk), "%d", s.rank);
            short r = (short)(S.font * 0.9f + 4);         // circle big enough to hold the numeral
            short cx = (S.rankpos == 1)                 // outside the tip
                       ? (short)(tip + sgn * (r + 4))
                       : (short)(tip - sgn * (r + 2));  // inside the tip
            RCT bub; bub.set((short)(cx - r), (short)(p.v - r), (short)(cx + r), (short)(p.v + r));
            setPen(C_DARK, 1, P_SOLID); CBRUSH bb(C_DARK, PAT_SOLID); bb.set(); bub.drawOval(DRAW_OPAQUE);
            // center the numeral exactly on the strike (cx, p.v) using measured metrics
            FONT f; f.id = HELVETICA; f.size = (short)(S.font - 1); f.style = BOLD; setFont(f);
            int lead=0, asc=0, desc=0; getFontMetrics(&lead, &asc, &desc);
            short tw = (short)getTextWidth(rk, -1);
            PNT tp; tp.h = (short)(cx - tw/2); tp.v = (short)(p.v + (asc - desc)/2);
            setTextColor(C_WHT); tp.drawText(rk);
        }

        // % OUTSIDE the tip (or inside), signed
        if (S.showpct && ab >= (float)S.hideu) {
            char pc[12]; sprintf_s(pc, sizeof(pc), "%s%d%%", s.pct > 0 ? "+" : "", (int)(s.pct + (s.pct>=0?0.5f:-0.5f)));
            if (S.pctpos == 1) {  // inside near base
                COLOR ic = inkOn(col);
                if (sgn < 0) textLJ((short)(tip + 4), p.v, pc, ic, S.font, false);
                else         textRJ((short)(tip - 4), p.v, pc, ic, S.font, false);
            } else {              // outside the tip
                if (sgn < 0) textRJ((short)(tip - 6), p.v, pc, C_TXT, S.font, false);
                else         textLJ((short)(tip + 6), p.v, pc, C_TXT, S.font, false);
            }
        }
    }

    // level rail
    if (S.kline) drawLevel(lastBar, paneL, paneR, 0, D_KING,  "",          S.extk, S);  // line only; the node labels KING
    if (S.cw)    drawLevel(lastBar, paneL, paneR, 1, C_PINK,  "CALL WALL", false,  S);
    if (S.pw)    drawLevel(lastBar, paneL, paneR, 2, C_PINK,  "PUT WALL",  false,  S);
    if (S.flip)  drawLevel(lastBar, paneL, paneR, 3, C_FLIPC, "FLIP",      false,  S);
    if (S.em)    drawLevel(lastBar, paneL, paneR, 4, C_CYAN,  "EM-H",      false,  S);
    if (S.em)    drawLevel(lastBar, paneL, paneR, 5, C_CYAN,  "EM-L",      false,  S);

    // secondary-book King (inert until a SPYKING row exists in the CSV)
    if (S.spyking && hasSpyKing) {
        PEN_STYLE ps = S.lstyle==1 ? P_DOT : (S.lstyle==2 ? P_DASH : P_SOLID);
        PNT probe; probe.set(lastBar, spyKingPx);
        hlinePx(probe.v, paneL, paneR, C_SPYK, ps);
        textLJ((short)(paneL + 4), (short)(probe.v - S.font - 2), "SPY KING", C_SPYK, S.font, false);
    }
}

// ---- draw() ---------------------------------------------------------------
int GammaProfile::draw(void)
{
    load();
    render(cfg);       // use the cached settings (read in parms callbacks, valid even when the dialog is closed)
    return RTX_OK;
}

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    GammaProfile *p = new GammaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("Gamma node profile + level rail (reads lsFlexLevels\\GammaProfile.csv)");
    p->setVersion("0.32");
    return p;
}
