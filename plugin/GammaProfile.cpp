/********************************************************************************
 *  GammaProfile.cpp  —  Investor/RT RTX extension  (gamma node profile)
 *
 *  Per-strike gamma histogram + level rail, drawn on the instrument price axis
 *  (INSTRUMENT_SCALE). Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv
 *
 *  CSV: STRIKE,<price>,<pctKing -100..100>,<rank>,<isKing 0|1>[,<type>]
 *       KING,<p>  CW,<p>  PW,<p>  FLIP,<p>  EMH,<p>  EML,<p>
 *
 *  Full settings panel (see setup()). Build: x64 Release, link irtsdkV143-x64.lib.
 ********************************************************************************/
#include "irtsdk.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>

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

static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}
static float luma(COLOR c){ return 0.2126f*((c>>16)&0xFF) + 0.7152f*((c>>8)&0xFF) + 0.0722f*(c&0xFF); }
static COLOR inkOn(COLOR c){ return luma(c) > 140.0f ? C_DARK : C_WHT; }

// ---- parameter indices (MUST match setup() declaration order) -------------
enum {
    P_WIDTH, P_SIDE, P_DETACH, P_THICK, P_ROUND,
    P_FILTER, P_THRESH, P_BELOW, P_SCALE,
    P_CPOS, P_CNEG, P_CMID, P_KINGCOL, P_AMP, P_TRANS,
    P_SHOWPCT, P_PCTPOS, P_HIDEU, P_RANK, P_RANKPOS, P_RANKSCOPE, P_TYPE, P_FONT,
    P_KLINE, P_CW, P_PW, P_FLIP, P_EM, P_LSTYLE, P_LPOS, P_EXTK, P_EXTN,
    P_HEADER, P_SPOT
};

struct Settings {
    int width, side, thick, filter, thresh, below, scale, kingcol;
    int pctpos, hideu, rankpos, rankscope, font, lstyle, lpos;
    bool detach, round, amp, trans, showpct, rank, type;
    bool kline, cw, pw, flip, em, extk, extn, header, spot;
    COLOR cpos, cneg, cmid;
};

struct GStrike { float price; float pct; int rank; bool king; std::string type; };

// ---------------------------------------------------------------------------
class GammaProfile : public cppExtension {
public:
    GammaProfile() : cppExtension() {}
    virtual int draw(void);

    std::vector<GStrike> strikes;
    float lvl[6]; bool has[6];   // KING,CW,PW,FLIP,EMH,EML
    float spotPx; bool hasSpot;
    std::string book;

    void load();
    void readSettings(Settings& S);
    void render(const Settings& S);
    void drawBar(short l, short t, short r, short b, COLOR col, bool rounded, bool trans);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX,  short y, const char* s, COLOR col, int sz, bool bold);
    void drawLevel(int lastBar, short lx, short rx, int idx, COLOR col, const char* label, const Settings& S);
};

// ---- base-vtable resolvers ------------------------------------------------
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- parameter panel ------------------------------------------------------
int cppExtension::setup(void)
{
    setParameterDialogHeight(46);
    // Layout
    setIntegerParameter("Width (px)", 130);
    setListParameter   ("Side", 0, "Right;Left");
    setBoolParameter   ("Detach from bars", true);
    setListParameter   ("Bar thickness", 0, "Auto;Thin;Medium;Thick");
    setBoolParameter   ("Rounded ends", true);
    // Nodes
    setListParameter   ("Filter", 1, "Top 3;Top 5;>= Threshold;All");
    setIntegerParameter("Threshold %", 20);
    setListParameter   ("Below threshold", 0, "Grey out;Hide");
    setListParameter   ("Scale bars to", 0, "King=100%;Visible max");
    // Color
    setColorParameter  ("+Gamma color", D_POS);
    setColorParameter  ("-Gamma color", D_NEG);
    setColorParameter  ("Midpoint color", D_MID);
    setListParameter   ("King colored by", 0, "Polarity;Distinct");
    setBoolParameter   ("Amplify polarity", false);
    setBoolParameter   ("Translucent bars", false);
    // Labels
    setBoolParameter   ("Show %", true);
    setListParameter   ("% position", 0, "Outside;Inside");
    setIntegerParameter("Hide % under (abs)", 5);
    setBoolParameter   ("Rank badge", true);
    setListParameter   ("Rank position", 0, "Inside;Outside");
    setListParameter   ("Rank scope", 0, "Top 5;Top 3");
    setBoolParameter   ("Node type inside", true);
    setIntegerParameter("Font size", 12);
    // Levels
    setBoolParameter   ("King line", true);
    setBoolParameter   ("Call Wall line", true);
    setBoolParameter   ("Put Wall line", true);
    setBoolParameter   ("Flip line", true);
    setBoolParameter   ("EM High/Low lines", true);
    setListParameter   ("Line style", 0, "Solid;Dot;Dash");
    setListParameter   ("Level label position", 0, "Left;Center;Right");
    setBoolParameter   ("Extend King line", false);
    setBoolParameter   ("Extend top-node lines", false);
    // Context
    setBoolParameter   ("Header", true);
    setBoolParameter   ("Spot marker", true);
    return RTX_OK;
}

// ---- read settings --------------------------------------------------------
void GammaProfile::readSettings(Settings& S)
{
    S.width = getIntegerValue(P_WIDTH); if (S.width < 40) S.width = 40; if (S.width > 400) S.width = 400;
    S.side  = getListIndex(P_SIDE);
    S.detach= isBoxChecked(P_DETACH) != 0;
    S.thick = getListIndex(P_THICK);
    S.round = isBoxChecked(P_ROUND) != 0;
    S.filter= getListIndex(P_FILTER);
    S.thresh= getIntegerValue(P_THRESH); if (S.thresh < 0) S.thresh = 0;
    S.below = getListIndex(P_BELOW);
    S.scale = getListIndex(P_SCALE);
    COLOR cp=(COLOR)(getIntegerValue(P_CPOS)&0xFFFFFF); S.cpos = cp ? cp : D_POS;
    COLOR cn=(COLOR)(getIntegerValue(P_CNEG)&0xFFFFFF); S.cneg = cn ? cn : D_NEG;
    COLOR cm=(COLOR)(getIntegerValue(P_CMID)&0xFFFFFF); S.cmid = cm ? cm : D_MID;
    S.kingcol  = getListIndex(P_KINGCOL);
    S.amp      = isBoxChecked(P_AMP) != 0;
    S.trans    = isBoxChecked(P_TRANS) != 0;
    S.showpct  = isBoxChecked(P_SHOWPCT) != 0;
    S.pctpos   = getListIndex(P_PCTPOS);
    S.hideu    = getIntegerValue(P_HIDEU); if (S.hideu < 0) S.hideu = 0;
    S.rank     = isBoxChecked(P_RANK) != 0;
    S.rankpos  = getListIndex(P_RANKPOS);
    S.rankscope= getListIndex(P_RANKSCOPE);   // 0=Top5, 1=Top3
    S.type     = isBoxChecked(P_TYPE) != 0;
    S.font     = getIntegerValue(P_FONT); if (S.font < 7) S.font = 7; if (S.font > 28) S.font = 28;
    S.kline = isBoxChecked(P_KLINE) != 0;
    S.cw    = isBoxChecked(P_CW) != 0;
    S.pw    = isBoxChecked(P_PW) != 0;
    S.flip  = isBoxChecked(P_FLIP) != 0;
    S.em    = isBoxChecked(P_EM) != 0;
    S.lstyle= getListIndex(P_LSTYLE);
    S.lpos  = getListIndex(P_LPOS);
    S.extk  = isBoxChecked(P_EXTK) != 0;
    S.extn  = isBoxChecked(P_EXTN) != 0;
    S.header= isBoxChecked(P_HEADER) != 0;
    S.spot  = isBoxChecked(P_SPOT) != 0;
}

// ---- data load ------------------------------------------------------------
void GammaProfile::load()
{
    for (int i = 0; i < 6; i++) { has[i] = false; lvl[i] = 0.0f; }
    hasSpot = false; spotPx = 0.0f; book = "SPX";
    const char* up = getenv("USERPROFILE");
    if (!up) { strikes.clear(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";
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
            if      (t[0] == "KING") { lvl[0]=v; has[0]=true; }
            else if (t[0] == "CW")   { lvl[1]=v; has[1]=true; }
            else if (t[0] == "PW")   { lvl[2]=v; has[2]=true; }
            else if (t[0] == "FLIP") { lvl[3]=v; has[3]=true; }
            else if (t[0] == "EMH")  { lvl[4]=v; has[4]=true; }
            else if (t[0] == "EML")  { lvl[5]=v; has[5]=true; }
            else if (t[0] == "SPOT") { spotPx=v; hasSpot=true; }
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

// ---- level rail -----------------------------------------------------------
void GammaProfile::drawLevel(int lastBar, short lx, short rx, int idx, COLOR col, const char* label, const Settings& S)
{
    if (!has[idx]) return;
    PEN_STYLE ps = S.lstyle==1 ? P_DOT : (S.lstyle==2 ? P_DASH : P_SOLID);
    setPen(col, 1, ps);
    PNT a; a.set(0, lvl[idx]);        a.setDrawPosition();
    PNT b; b.set(lastBar, lvl[idx]);  b.drawLineTo();
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

    // right edge of the last price bar (for detach)
    PNT lb; lb.set(lastBar, strikes[0].price);
    short lastX = (short)(lb.h + getPixelsPerBar()/2);

    // effective width / anchor
    short w = (short)S.width;
    short anchor;                 // base of bars
    int sgn;                      // +1 grows right, -1 grows left
    if (S.side == 1) {            // Left margin: anchor at pane left, grow right
        anchor = (short)(paneL + 2);
        sgn = +1;
        if (S.detach) { short room = (short)(lastX - anchor - 6); if (room > 30 && room < w) w = room; }
    } else {                      // Right margin (default): anchor at pane right, grow left
        anchor = paneR;
        sgn = -1;
        if (S.detach) { short room = (short)(paneR - lastX - 6); if (room > 30 && room < w) w = room; }
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

    // bars
    for (size_t i = 0; i < strikes.size(); i++) {
        GStrike& s = strikes[i];
        float ab = std::fabs(s.pct);

        bool primary;
        if      (S.filter == 0) primary = (s.rank >= 1 && s.rank <= 3);
        else if (S.filter == 1) primary = (s.rank >= 1 && s.rank <= 5);
        else if (S.filter == 2) primary = (ab >= (float)S.thresh);
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

        short top = (short)(p.v - barH/2), bot = (short)(p.v + barH/2);
        short bl = sgn < 0 ? tip : anchor, br = sgn < 0 ? anchor : tip;
        drawBar(bl, top, br, bot, col, S.round, S.trans);

        // node TYPE inside, near the base
        if (S.type && !s.type.empty() && len > 34) {
            COLOR ic = inkOn(col);
            if (sgn < 0) textRJ((short)(anchor - 6), p.v, s.type.c_str(), ic, S.font, true);
            else         textLJ((short)(anchor + 6), p.v, s.type.c_str(), ic, S.font, true);
        }

        // RANK bubble
        bool inScope = (S.rankscope == 1) ? (s.rank>=1 && s.rank<=3) : (s.rank>=1 && s.rank<=5);
        if (S.rank && inScope) {
            char rk[8]; sprintf_s(rk, sizeof(rk), "%d", s.rank);
            short r = (short)(S.font/2 + 5);
            short cx = (S.rankpos == 1)                 // outside the tip
                       ? (short)(tip + sgn * (r + 4))
                       : (short)(tip - sgn * (r + 2));  // inside the tip
            RCT bub; bub.set((short)(cx - r), (short)(p.v - r), (short)(cx + r), (short)(p.v + r));
            setPen(C_DARK, 1, P_SOLID); CBRUSH bb(C_DARK, PAT_SOLID); bb.set(); bub.drawOval(DRAW_OPAQUE);
            FONT f; f.id = HELVETICA; f.size = (short)S.font; f.style = BOLD; setFont(f);
            setTextColor(C_WHT); bub.drawText(rk, true, false);
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
    if (S.kline) drawLevel(lastBar, paneL, paneR, 0, D_KING,  "",          S);  // line only; the node labels KING
    if (S.cw)    drawLevel(lastBar, paneL, paneR, 1, C_PINK,  "CALL WALL", S);
    if (S.pw)    drawLevel(lastBar, paneL, paneR, 2, C_PINK,  "PUT WALL",  S);
    if (S.flip)  drawLevel(lastBar, paneL, paneR, 3, C_FLIPC, "FLIP",      S);
    if (S.em)    drawLevel(lastBar, paneL, paneR, 4, C_CYAN,  "EM-H",      S);
    if (S.em)    drawLevel(lastBar, paneL, paneR, 5, C_CYAN,  "EM-L",      S);
}

// ---- draw() ---------------------------------------------------------------
int GammaProfile::draw(void)
{
    load();
    Settings S; readSettings(S);
    render(S);
    return RTX_OK;
}

// ---- factory --------------------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    GammaProfile *p = new GammaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("IRT Gamma Profile \x97 per-strike gamma histogram + level rail, tape-matched, with full settings. Reads lsFlexLevels\\GammaProfile.csv");
    p->setVersion("0.2");
    return p;
}
