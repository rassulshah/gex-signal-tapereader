/********************************************************************************
 *  GammaProfile.cpp  —  Investor/RT RTX extension  (gamma node profile)
 *
 *  Per-strike gamma histogram + level rail, drawn on the instrument price axis
 *  (INSTRUMENT_SCALE). Reads %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv
 *  (or GammaProfile-SPY.csv when Book = SPY).
 *
 *  CSV: STRIKE,<price>,<pctKing -100..100>,<rank>,<isKing 0|1>,<type>,<spx strike>
 *       KING,<p>  CW,<p>,<spx>,<win>,<src>  PW,...  FLIP,...  EMH,<p>  EML,<p>  SPOT,<p>
 *       CW/PW,<es>,<spx>,<win>,<src>[,<depth 0D|WK|MO>,<s0>,<sW>]  SLOPE,<word>,<sDn>,<sUp>,<rDn>,<rUp>
 *       SCALEREF,<front ES>[,<CT sod>,<CT date>]  REGIME,<sign>,<type>,<conf>,<conflict>,<flip spx>,<note>
 *       SPYKING,<p>   BOOK,<name>   ASOF,<sec>
 *
 *  Full settings panel (see setup()). Build: x64 Release, link irtsdkV143-x64.lib.
 *
 *  v0.40 — CONTRACT ALIGNMENT anchors on the new SCALEREF row (the front-month ES
 *  price the ladder is scaled to) instead of SPOT, so the King/nodes land on the
 *  charted contract during the quarterly roll (EPZ26 Dec ~+70 over front); spot is
 *  pinned to the chart's live close for the marker and the Gatekeeper role test.
 *  v0.55 — the pill's text drawn like its neighbours (textLJ), the box centred where those glyphs actually land.
 *  v0.54 — the depth pill drawn like the rank bubble (measured box, metric-placed text); one size up.
 *  v0.53 — Book defaults to IF (one rail, switch the Book dropdown to compare; operator 2026-09-16).
 *  v0.52 — the Magnet (King) on the chip's second line, right after the PW (operator, 21:40).
 *  v0.51 — pill placement: outward order bubble · tag · pill · % (the pill had covered the % label); chip gaps measured.
 *  v0.50 — IF extras: the wall DEPTH pill (0D / WK / MO from the CW / PW rows' 6th field) beside the node tag and on
 *          the chip's second line; the SLOPE row's word (STEEP dn / STEEP up / flat) after the sign on the chip's first.
 *  v0.49 — SCALEREF,<px>,<sod>,<date> (panel 16.34): the offset anchors on the bar of the quote's OWN minute — Skylit's
 *          series froze at 14:26 CT on FOMC 2026-09-16, so even the 15:00 bar was wrong by the last half hour's move.
 *  v0.48 — contract offset anchored on the last RTH bar at or before ASOF (SCALEREF freezes at the cash close;
 *          the evening / pre-open chart moves on — the book drew 22 pts high after hours on 2026-09-16).
 *  v0.47 — Book: IF reads GammaProfile-IF.csv (InsiderFinance 0DTE chain, same grammar). The
 *  header prints "IF 0DTE" from the BOOK row. Nothing else differs between the two books on the rail.
 *  v0.42 — DOCTRINE LABELS + REGIME ROW + LEVEL CALLOUTS (operator, 2026-09-15/16):
 *    · Gatekeeper = ONE node: the largest |%King| strictly between spot and the King,
 *      and only if >= GK_MIN_PCT (30% of King). No more "G on every node" flood.
 *    · Air pocket = a thin run (>=3 strikes under AIR_THIN_PCT) BOUNDED on BOTH sides
 *      by a significant node (>= AIR_EDGE_PCT); the far-OTM tail is never banded.
 *    · Pattern tags (P/B/R/RR) arrive in the STRIKE row's 6th field from the panel and
 *      draw INSIDE the bar even when a role tag (K/C/F/G) sits at the base.
 *    · REGIME row from the panel (sign = spot vs IF 0DTE flip; type = Range/Trend/
 *      Whipsaw from the Skylit structure read) replaces the distance-blind sum of
 *      %King; the sum is only a labelled fallback when the row is absent.
 *    · "Level labels" (appended LAST): CW / PW tags on the wall nodes and a FLIP tick
 *      on the bar strip, as an alternative to (or alongside) the level LINES.
 *
 *  Parameter indices are numbered explicitly (pc++), one per control, with NO
 *  setLabelParameter section headers -- a label row shifts IRT's parameter
 *  numbering and silently scrambles every setting read after it.
 ********************************************************************************/
#include "irtsdk.h"
#include "GammaProfileLogic.h"     // (v0.45) the decisions, testable without IRT
#include "ContractOffsetLogic.h"   // (shared) the anchor-bar rule, tested
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>

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
static const COLOR C_SUP  = 0x003FB27A;  // support side stripe (green, below spot)
static const COLOR C_RES  = 0x00D15B6B;  // resistance side stripe (red, above spot)
static const COLOR C_AIR  = 0x003A4658;  // air-pocket band (translucent grey)
static const COLOR C_AIRN = 0x00512A48;  // (v0.42) air-pocket band when the run leans -gamma (violent pathway)
// (v0.45) the doctrine thresholds and every decision now live in GammaProfileLogic.h (gpl::), unit-tested.

static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}
static float luma(COLOR c){ return 0.2126f*((c>>16)&0xFF) + 0.7152f*((c>>8)&0xFF) + 0.0722f*(c&0xFF); }
static COLOR inkOn(COLOR c){ return luma(c) > 140.0f ? C_DARK : C_WHT; }
// (v0.45) pattern tags: gpl::patternTag() — P / B on the named stack member, R / RR on the rug.

// ---- parameter indices, filled in setup() (single-instance -> file static) --
struct PIdx {
    int book, width, side, thick, detach, round;
    int filter, thresh, below, scale;
    int cpos, cneg, cmid, kingcol, amp, trans;
    int showpct, pctpos, hideu, rank, rankpos, rankscope, type, kinglabel, font;
    int kline, cw, pw, flip, em, lstyle, lpos, extk, topnodes, topstyle, spyking;
    int header, spot;
    int roles, regime, panelpos, defbands, confl, legend;   // structure read (appended v0.35)
    int headerpos;                                          // header placement (appended v0.36)
    int tapecols;                                           // Skylit-style [SPX strike | %King] columns (appended v0.41)
    int lvllabels;                                          // CW/PW tags on the wall nodes + FLIP tick on the strip (appended v0.42)
};
static PIdx PX;

struct Settings {
    int book, width, side, thick, filter, thresh, below, scale, kingcol;
    int pctpos, hideu, rankpos, rankscope, font, lstyle, lpos, topstyle;
    bool detach, round, amp, trans, showpct, rank, type;
    bool kline, cw, pw, flip, em, extk, topnodes, spyking, header, spot;
    bool roles, regime, defbands, confl, legend; int panelpos;   // structure read
    int headerpos;
    bool tapecols;
    bool lvllabels;
    COLOR cpos, cneg, cmid;
    char kinglabel[24];
};

struct GStrike { float price; float pct; int rank; bool king; std::string type; float spx; };   // spx = raw SPXW strike (v0.41)

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
    float lvlSpx[6];             // (v0.42) the raw SPX strike each level row carried (0 = none) — matches nodes by strike
    std::string lvlWin[6];       // (v0.42) the window the row was computed in ("0DTE"), for the label
    // (v0.42) REGIME row from the panel: REGIME,<sign NEG|POS|AT>,<type>,<conf>,<conflict 0|1>,<flip spx>,<note>
    bool hasRegime; std::string rgSign, rgType, rgConf, rgNote; bool rgConflict; float rgFlipSpx;
    std::string lvlDepth[6];     // (v0.50) the wall's depth tag from the CW / PW row (0D / WK / MO), empty = none
    std::string slopeWord;       // (v0.50) the SLOPE row's word (STEEP dn / STEEP up / flat), empty = none
    float spotPx; bool hasSpot;
    float scaleRef; bool hasScaleRef;   // (v0.40) front-month ES anchor the ladder is scaled to (SCALEREF row)
    double scaleRefSo; int scaleRefY, scaleRefM, scaleRefD;   // (v0.49) the CT minute + date of that quote (panel 16.34); so<0 = unknown
    float spyKingPx; bool hasSpyKing;
    std::string book;
    double asofSo;               // (v0.38) ASOF write-time (CT sec-of-day) for the STALE badge; <0 = unknown
    Settings cfg;                // cached settings (populated in parms callbacks, used in draw)

    void load();
    void drawStaleBadge();       // (v0.38) red badge if the CSV has gone cold (shared across all 4 plugins)
    void applyContractOffset();  // (v0.39) shift the whole book onto the chart's own contract price
    void readSettings(Settings& S);
    void render(const Settings& S);
    void drawBar(short l, short t, short r, short b, COLOR col, bool rounded, bool trans);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX,  short y, const char* s, COLOR col, int sz, bool bold);
    short pillW(const std::string& tag, const Settings& S);                       // (v0.50)
    short drawDepthPill(short leftX, short y, const std::string& tag, const Settings& S);   // (v0.50) returns its width (0 when no tag)
    void textC (short cx,     short y, const char* s, COLOR col, int sz, bool bold);
    void hlinePx(short y, short lx, short rx, COLOR col, PEN_STYLE ps);
    void bandPrice(float p1, float p2, short lx, short rx, COLOR col);
    void drawPanel(RCT pane, const Settings& S, float net, int fIdx, int cIdx, int kIdx);
    void drawLegend(RCT pane, const Settings& S);
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
    cfg.book=3; cfg.width=100; cfg.side=0; cfg.detach=true; cfg.thick=0; cfg.round=true;
    cfg.filter=1; cfg.thresh=20; cfg.below=0; cfg.scale=0;
    cfg.cpos=D_POS; cfg.cneg=D_NEG; cfg.cmid=D_MID; cfg.kingcol=0; cfg.amp=false; cfg.trans=false;
    cfg.showpct=true; cfg.pctpos=0; cfg.hideu=5; cfg.rank=true; cfg.rankpos=0; cfg.rankscope=0;
    cfg.type=true; cfg.font=10;
    strncpy(cfg.kinglabel, "K", sizeof(cfg.kinglabel)); cfg.kinglabel[sizeof(cfg.kinglabel)-1]=0;
    cfg.kline=true; cfg.cw=true; cfg.pw=true; cfg.flip=true; cfg.em=false;
    cfg.lstyle=0; cfg.lpos=0; cfg.extk=false; cfg.topnodes=false; cfg.topstyle=1; cfg.spyking=false;
    cfg.header=false; cfg.spot=true; cfg.headerpos=0; cfg.tapecols=false; cfg.lvllabels=true;
    cfg.roles=true; cfg.regime=true; cfg.panelpos=1; cfg.defbands=false; cfg.confl=false; cfg.legend=false;
}

// ---- parameter callbacks: dialog controls are valid here, so read + cache.
// parmsLoad is guarded: only accept the read if a control reads a plausible value,
// otherwise keep the constructor defaults (so the profile always renders).
// All three guard on a plausible font read: if the dialog somehow hands back a
// nonsense value we keep the cached settings rather than corrupt the render.
// Threshold % is only meaningful when Show = ">= Threshold" (filter index 4);
// grey it out otherwise so it doesn't read like a second live control.
int GammaProfile::parmsLoad(void)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) { readSettings(cfg); enableParameter(PX.thresh, cfg.filter == 4); }
    return RTX_OK;
}
int GammaProfile::parmsApply(void)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) readSettings(cfg);
    return RTX_OK;
}
int GammaProfile::parmsUpdt(unsigned int)
{
    int probe = getIntegerValue(PX.font);
    if (probe >= 6 && probe <= 48) { readSettings(cfg); enableParameter(PX.thresh, cfg.filter == 4); }
    return RTX_OK;
}

// ---- parameter panel ------------------------------------------------------
// IMPORTANT: parameter indices are numbered EXPLICITLY here (pc++), one per
// control. Do NOT use setLabelParameter for section headers -- a label row
// shifts IRT's internal parameter numbering out from under the value getters
// (getListIndex / isBoxChecked / getIntegerValue), which silently scrambles
// every setting declared after it. That bug greyed nodes, dropped labels and
// reset Width. Controls only, counted 1:1, is the reliable scheme.
// Two-per-row layout is via kParmAppendSameLine (pure layout; each same-line
// control still gets its own index, so it does not affect numbering).
int cppExtension::setup(void)
{
    setParameterVersion(5);
    setParameterDialogHeight(26);

    const short SL = kParmAppendSameLine;
    int pc = 0;                        // the TRUE parameter index, one per control

    // PROFILE
    PX.book   = pc++; setListParameter   ("Book", 3, "Auto;SPX;SPY;IF");   // (v0.47) IF = the InsiderFinance 0DTE book, GammaProfile-IF.csv · (v0.53) IF is the DEFAULT: operator 2026-09-16, "one or the other, switchable; IF as default for now"
    PX.width  = pc++; setIntegerParameter("Width px", 100, 0, SL);
    PX.side   = pc++; setListParameter   ("Side", 0, "Right;Left");
    PX.thick  = pc++; setListParameter   ("Thickness", 0, "Auto;Thin;Medium;Thick", 0, SL);
    PX.detach = pc++; setBoolParameter   ("Detach from bars", true);
    PX.round  = pc++; setBoolParameter   ("Rounded ends", true, SL);
    // NODES
    PX.filter = pc++; setListParameter   ("Show", 1, "Top 3;Top 5;Top 8;Top 10;>= Threshold;All");
    PX.thresh = pc++; setIntegerParameter("Threshold %", 20, 0, SL);
    PX.below  = pc++; setListParameter   ("Sub-threshold", 0, "Grey out;Hide");
    PX.scale  = pc++; setListParameter   ("Scale to", 0, "King=100%;Visible max", 0, SL);
    // COLOR
    PX.cpos   = pc++; setColorParameter  ("+Gamma", D_POS);
    PX.cneg   = pc++; setColorParameter  ("-Gamma", D_NEG, 0, SL);
    PX.cmid   = pc++; setColorParameter  ("Midpoint", D_MID);
    PX.kingcol= pc++; setListParameter   ("King colour", 0, "Polarity;Distinct", 0, SL);
    PX.amp    = pc++; setBoolParameter   ("Amplify polarity", false);
    PX.trans  = pc++; setBoolParameter   ("Translucent bars", false, SL);
    // LABELS
    PX.showpct= pc++; setBoolParameter   ("Show %", true);
    PX.pctpos = pc++; setListParameter   ("% at", 0, "Outside;Inside", 0, SL);
    PX.hideu  = pc++; setIntegerParameter("Hide % under", 5);
    PX.font   = pc++; setIntegerParameter("Font size (pt)", 10, 0, SL);
    PX.rank   = pc++; setBoolParameter   ("Rank badge", true);
    PX.type   = pc++; setBoolParameter   ("Node name inside", true, SL);
    PX.rankpos= pc++; setListParameter   ("Rank at", 0, "Inside;Outside");
    PX.rankscope=pc++;setListParameter   ("Rank for (non-TopN filters)", 0, "Top 5;Top 3", 0, SL);
    PX.kinglabel=pc++;setListParameter   ("King name", 0, "K;KING;GPoc;GPOC;GEX;POC;GAMMA");
    // LEVELS -- the five line toggles on one row, as requested
    PX.kline  = pc++; setBoolParameter   ("King line", true);
    PX.cw     = pc++; setBoolParameter   ("Call Wall", true, SL);
    PX.pw     = pc++; setBoolParameter   ("Put Wall", true, SL);
    PX.flip   = pc++; setBoolParameter   ("Flip", true, SL);
    PX.em     = pc++; setBoolParameter   ("EM H/L", false, SL);
    PX.extk   = pc++; setBoolParameter   ("Extend King line", false);
    PX.topnodes=pc++; setBoolParameter   ("Top-node lines", false, SL);
    PX.lstyle = pc++; setListParameter   ("Line style", 0, "Solid;Dot;Dash");
    PX.lpos   = pc++; setListParameter   ("Label at", 0, "Left;Center;Right", 0, SL);
    PX.topstyle=pc++; setListParameter   ("Top-node style", 1, "Solid;Dot;Dash");
    PX.spyking= pc++; setBoolParameter   ("SPY King line", false, SL);
    // CONTEXT
    PX.header = pc++; setBoolParameter   ("Title header (top-left)", false);
    PX.spot   = pc++; setBoolParameter   ("Spot price line (dotted)", true, SL);
    // STRUCTURE READ (appended v0.35 -- new params go at the END so existing
    // instances keep their numbering)
    PX.roles   = pc++; setBoolParameter  ("Structure labels (Floor/Ceiling/Gate/Air)", true);
    PX.regime  = pc++; setBoolParameter  ("Regime + read panel", true, SL);
    PX.panelpos= pc++; setListParameter  ("Panel at", 1, "Bottom-L;Bottom-C;Bottom-R;Top-L;Top-C;Top-R");
    PX.defbands= pc++; setBoolParameter  ("Deflection bands", false, SL);
    PX.confl   = pc++; setBoolParameter  ("EM confluence marks", false);
    PX.legend  = pc++; setBoolParameter  ("Polarity legend", false, SL);
    PX.headerpos=pc++; setListParameter  ("Header at", 0, "Top-L;Top-C;Top-R;Bottom-L;Bottom-C;Bottom-R");
    PX.tapecols =pc++; setBoolParameter  ("Tape columns (SPX strike | %King, right edge)", false);   // (v0.41) appended
    PX.lvllabels=pc++; setBoolParameter  ("Level labels on nodes (CW / PW / FLIP)", true);           // (v0.42) appended LAST
    return RTX_OK;
}

// ---- self-diagnostic: append what we actually read to a file the bridge can
// stage, so the parameter mapping can be VERIFIED empirically, not assumed.
// (Writes GammaProfile.debug.txt beside the CSV; harmless, deletable.)
static void dbgDump(const char* when, const Settings& S)
{
    const char* up = getenv("USERPROFILE");
    if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.debug.txt";
    std::ofstream f(path.c_str(), std::ios::app);
    if (!f.is_open()) return;
    f << "v0.42 " << when
      << " | IDX font="   << PX.font   << " hideu=" << PX.hideu << " showpct=" << PX.showpct
      << " rank="         << PX.rank   << " type="  << PX.type  << " filter="  << PX.filter
      << " width="        << PX.width
      << " | READ font="  << S.font    << " hideu=" << S.hideu  << " showpct=" << (int)S.showpct
      << " rank="         << (int)S.rank << " type=" << (int)S.type << " filter=" << S.filter
      << " width="        << S.width   << " thresh=" << S.thresh << " detach=" << (int)S.detach
      << "\n";
    f.close();
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
    S.roles   = isBoxChecked(PX.roles) != 0;
    S.regime  = isBoxChecked(PX.regime) != 0;
    S.panelpos= getListIndex(PX.panelpos);
    S.defbands= isBoxChecked(PX.defbands) != 0;
    S.confl   = isBoxChecked(PX.confl) != 0;
    S.legend  = isBoxChecked(PX.legend) != 0;
    S.headerpos = getListIndex(PX.headerpos);
    S.tapecols  = isBoxChecked(PX.tapecols) != 0;
    S.lvllabels = isBoxChecked(PX.lvllabels) != 0;
    dbgDump("read", S);   // record what was actually read (diagnostic)
}

// ---- data load ------------------------------------------------------------
void GammaProfile::load()
{
    for (int i = 0; i < 6; i++) { has[i] = false; lvl[i] = 0.0f; lvlSpx[i] = 0.0f; lvlWin[i].clear(); lvlDepth[i].clear(); }
    slopeWord.clear();
    hasRegime = false; rgSign.clear(); rgType.clear(); rgConf.clear(); rgNote.clear(); rgConflict = false; rgFlipSpx = 0.0f;
    hasSpot = false; spotPx = 0.0f; hasSpyKing = false; spyKingPx = 0.0f; book = "SPX"; asofSo = -1; scaleRefSo = -1; scaleRefY = scaleRefM = scaleRefD = 0;
    hasScaleRef = false; scaleRef = 0.0f;
    const char* up = getenv("USERPROFILE");
    if (!up) { strikes.clear(); return; }
    // Book selector: Auto(0) and SPX(1) read GammaProfile.csv (the Skylit tape); SPY(2) GammaProfile-SPY.csv;
    // (v0.47) IF(3) GammaProfile-IF.csv — the InsiderFinance 0DTE chain in the same row grammar (design/IF-BOOK-OPTION.md).
    // A second lsGammaProfile instance with Book = IF and Side = Left puts the two books side by side on one rail.
    const char* fname = (cfg.book == 2) ? "GammaProfile-SPY.csv" : (cfg.book == 3 ? "GammaProfile-IF.csv" : "GammaProfile.csv");
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
            s.spx   = (t.size() >= 7) ? (float)atof(t[6].c_str()) : 0.0f;   // (v0.41) raw SPXW strike, 0 if the panel predates it
            tmp.push_back(s);
        } else if (t.size() >= 2) {
            float v = (float)atof(t[1].c_str());
            // (v0.42) level rows may carry: <es price>,<spx strike>,<window>,<src>. Older panels write only the price.
            if      (t[0] == "KING")    { lvl[0]=v; has[0]=true; }
            else if (t[0] == "CW" || t[0] == "PW" || t[0] == "FLIP") {
                int li = (t[0] == "CW") ? 1 : (t[0] == "PW" ? 2 : 3);
                lvl[li]=v; has[li]=true;
                if (t.size() >= 3) lvlSpx[li] = (float)atof(t[2].c_str());
                if (t.size() >= 4) lvlWin[li] = t[3];
                if (t.size() >= 6 && (t[5] == "0D" || t[5] == "WK" || t[5] == "MO")) lvlDepth[li] = t[5];   // (v0.50) depth tag
            }
            else if (t[0] == "SLOPE" && t.size() >= 2) { slopeWord = t[1]; }   // (v0.50) the curve's slope, one word
            else if (t[0] == "REGIME") {
                // REGIME,<sign>,<type>,<conf>,<conflict>,<flip spx>,<note...>
                hasRegime = true;
                rgSign = t[1];
                rgType = (t.size() >= 3) ? t[2] : std::string();
                rgConf = (t.size() >= 4) ? t[3] : std::string();
                rgConflict = (t.size() >= 5) && atoi(t[4].c_str()) != 0;
                rgFlipSpx = (t.size() >= 6) ? (float)atof(t[5].c_str()) : 0.0f;
                rgNote.clear(); for (size_t q = 6; q < t.size(); q++) { if (q > 6) rgNote += ","; rgNote += t[q]; }
            }
            else if (t[0] == "EMH")     { lvl[4]=v; has[4]=true; }
            else if (t[0] == "EML")     { lvl[5]=v; has[5]=true; }
            else if (t[0] == "SPOT")    { spotPx=v; hasSpot=true; }
            else if (t[0] == "SCALEREF"){ scaleRef=v; hasScaleRef=true; scaleRefSo=-1; scaleRefY=scaleRefM=scaleRefD=0;
                if (t.size() >= 4) { scaleRefSo = atof(t[2].c_str()); sscanf(t[3].c_str(), "%d-%d-%d", &scaleRefY, &scaleRefM, &scaleRefD); } }   // (v0.49) the quote's own minute
            else if (t[0] == "SPYKING") { spyKingPx=v; hasSpyKing=true; }
            else if (t[0] == "BOOK" && t.size()>=2) { book=t[1]; }
            else if (t[0] == "ASOF") { asofSo = v; }
        }
    }
    strikes.swap(tmp);
}

// ---- small drawing helpers ------------------------------------------------
void GammaProfile::drawBar(short l, short t, short r, short b, COLOR col, bool rounded, bool trans)
{
    // "Translucent" = fade the bar toward the dark chart ground so candles/grid
    // read through it. Deterministic (no reliance on a host translucent mode)
    // and it keeps rounded ends.
    COLOR c = trans ? lerpColor(col, C_DARK, 0.55f) : col;
    RCT rc; rc.set(l, t, r, b);
    if (rounded) {
        setPen(c, 1, P_SOLID);
        CBRUSH br(c, PAT_SOLID); br.set();
        rc.drawRounded(6, 6);
    } else {
        rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
    }
}
void GammaProfile::textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    RCT rc; rc.set((short)(rightX - 260), (short)(y - sz), rightX, (short)(y + sz));
    rc.drawText(s, false, true);      // not centered, right-justified
}
// (v0.50) THE DEPTH PILL — a small bordered box with the tag in the depth colour: 0D pink (this session only), WK amber
// (holds through the week), MO cyan (the monthly book sits on it). Drawn beside the CW / PW tag on the node and after
// the wall on the chip's second line. Colours are the mockup's; the tag comes from the panel (gpWallDepth).
static COLOR depthColour(const std::string& tag) { return tag == "0D" ? 0x00AF3BC4 : (tag == "WK" ? 0x0041C3E3 : 0x00E0D04F); }
short GammaProfile::pillW(const std::string& tag, const Settings& S)
{
    if (tag.empty()) return 0;
    FONT f; f.id = HELVETICA; f.size = (short)(S.font - 1); f.style = BOLD; setFont(f);
    return (short)(getTextWidth(tag.c_str(), (int)tag.size()) + 10);
}
short GammaProfile::drawDepthPill(short leftX, short y, const std::string& tag, const Settings& S)
{
    if (tag.empty()) return 0;
    // (v0.55) THE TEXT IS DRAWN EXACTLY LIKE ITS NEIGHBOURS (textLJ at the same y), so it can never sit on a different
    // line from "PW" or "PW 7624 (7550)". Measured on his 0.54 screenshot: textLJ's glyphs land ~0.45 x font BELOW y
    // (the rect draw baselines low), so the box is centred there, not on y — the metric-placed text of 0.54 sat 4 px
    // above everything beside it.
    short w = pillW(tag, S), h = (short)(S.font + 5);
    short cy = (short)(y + (short)(S.font * 0.45f + 0.5f));
    COLOR c = depthColour(tag);
    RCT box; box.set(leftX, (short)(cy - h/2), (short)(leftX + w), (short)(cy + h/2));
    box.draw(1, c, C_DARK, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(leftX + 5), y, tag.c_str(), c, S.font - 1, true);
    return w;
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
// Translucent price band across [lx,rx] between two price levels.
void GammaProfile::bandPrice(float p1, float p2, short lx, short rx, COLOR col)
{
    PNT a; a.set(0, p1); PNT b; b.set(0, p2);
    short y1 = a.v < b.v ? a.v : b.v, y2 = a.v < b.v ? b.v : a.v;
    RCT rc; rc.set(lx, y1, rx, y2);
    rc.draw(0, col, col, DRAW_TRANSLUCENT, PAT_SOLID);
}
// Regime + read panel, anchored to any of six pane positions ("Panel at").
// Dataviz: a colored swatch carries the polarity identity; the text stays in
// neutral ink so it reads cleanly on the dark box.
void GammaProfile::drawPanel(RCT pane, const Settings& S, float net, int fIdx, int cIdx, int kIdx)
{
    short lineH = (short)(S.font + 8);
    short pw = (short)(S.font * 34), ph = (short)(2*lineH + 16);   // (v0.42) wide enough for the regime line
    short L = (short)(pane.left + 8);
    short C = (short)((pane.left + pane.right)/2 - pw/2);
    short R = (short)(pane.right - pw - 8);
    short B = (short)(pane.bottom - ph - 8);
    short T = (short)(pane.top + 8);
    short x = (S.panelpos%3==0) ? L : (S.panelpos%3==1 ? C : R);   // 0 L, 1 C, 2 R
    short y = (S.panelpos < 3)  ? B : T;                           // 0-2 bottom, 3-5 top

    RCT box; box.set(x, y, (short)(x+pw), (short)(y+ph));
    box.draw(1, C_GREY, C_DARK, DRAW_OPAQUE, PAT_SOLID);

    // (v0.42) THE REGIME ROW FROM THE PANEL, when present. Sign = spot vs InsiderFinance's 0DTE zero-gamma
    // (the flip), type = the Skylit structure read (Range / Trend / Whipsaw — learn/gamma-regimes), conflict =
    // the two books disagree (surfaced, never resolved here). The old "sum of every %King" is only the fallback
    // for a CSV that predates the row, and it says so.
    gpl::RegimeText RT = gpl::regimeLine(hasRegime, rgSign, rgType, rgConflict, net);
    // (v0.50) the curve's slope word rides after the sign: "REGIME  -gamma STEEP dn | RANGE | ..." (IF extras #2)
    std::string rl = RT.line;
    if (!slopeWord.empty() && !RT.na) { size_t bar = rl.find(" | "); if (bar != std::string::npos) rl.insert(bar, " " + slopeWord); }
    bool neg = RT.neg; char l0[200]; sprintf_s(l0, sizeof(l0), "%s", rl.c_str());
    COLOR rcol = neg ? S.cneg : S.cpos;
    if (RT.at) rcol = C_FLIPC;
    if (RT.na) rcol = C_GREY;   // no flip / no spot: no sign call
    short ly0 = (short)(y + 9 + lineH/2);
    RCT sw; sw.set((short)(x+10), (short)(ly0-5), (short)(x+21), (short)(ly0+5));
    sw.draw(0, rcol, rcol, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x+28), ly0, l0, rgConflict ? 0x00FF9A8F : C_TXT, S.font, true);

    // (v0.44) THE SECOND LINE IS THE THREE IF LEVELS — operator, 2026-09-16: "it has levels at the bottom, those levels
    // should be Put Wall, Flip, Call Wall." Chart-scale price first (what the axis shows), the SPX strike in brackets
    // (what Skylit / IF print), each in its level colour. A missing row prints n/a rather than borrowing a node.
    char l1[200]; char pwS[48]="PW n/a", flS[48]="FLIP n/a", cwS[48]="CW n/a", kgS[48]="";
    // (v0.52) THE MAGNET (KING) AFTER THE PW — operator, 2026-09-16 21:40: "add the Magnet (King) to the Regime Chip, right
    // after PW." Chart price from the KING row (already offset onto this contract), the SPX strike from the King node; the
    // label is the configured King name (K / KING / GPoc ...), in the King's own colour.
    if (has[0] && kIdx >= 0 && kIdx < (int)strikes.size()) {
        float kspx = strikes[kIdx].spx;
        if (kspx > 0) sprintf_s(kgS, sizeof(kgS), "%s %d (%d)", S.kinglabel, (int)(lvl[0]+0.5f), (int)(kspx+0.5f));
        else          sprintf_s(kgS, sizeof(kgS), "%s %d", S.kinglabel, (int)(lvl[0]+0.5f));
    }
    if (has[2]) { if (lvlSpx[2] > 0) sprintf_s(pwS, sizeof(pwS), "PW %d (%d)",   (int)(lvl[2]+0.5f), (int)(lvlSpx[2]+0.5f)); else sprintf_s(pwS, sizeof(pwS), "PW %d",   (int)(lvl[2]+0.5f)); }
    if (has[3]) { if (lvlSpx[3] > 0) sprintf_s(flS, sizeof(flS), "FLIP %d (%d)", (int)(lvl[3]+0.5f), (int)(lvlSpx[3]+0.5f)); else sprintf_s(flS, sizeof(flS), "FLIP %d", (int)(lvl[3]+0.5f)); }
    if (has[1]) { if (lvlSpx[1] > 0) sprintf_s(cwS, sizeof(cwS), "CW %d (%d)",   (int)(lvl[1]+0.5f), (int)(lvlSpx[1]+0.5f)); else sprintf_s(cwS, sizeof(cwS), "CW %d",   (int)(lvl[1]+0.5f)); }
    short ly1 = (short)(y + 9 + lineH + lineH/2);
    // three coloured segments, laid out left to right with measured widths
    FONT lf; lf.id = HELVETICA; lf.size = (short)S.font; lf.style = PLAIN; setFont(lf);
    short gap = (short)(S.font * 2), cx1 = (short)(x + 10);
    // (v0.51) widths measured in the font the text is drawn with (textLJ sets S.font PLAIN), a real gap before the pill
    textLJ(cx1, ly1, pwS, C_SUP,   S.font, false); setFont(lf); cx1 = (short)(cx1 + getTextWidth(pwS, -1) + 8);
    if (has[2] && !lvlDepth[2].empty()) cx1 = (short)(cx1 + drawDepthPill(cx1, ly1, lvlDepth[2], S) + gap); else cx1 = (short)(cx1 + gap - 8);
    if (kgS[0]) { COLOR kc = (S.kingcol == 1) ? D_KING : ((kIdx >= 0 && strikes[kIdx].pct < 0) ? S.cneg : S.cpos);
                  textLJ(cx1, ly1, kgS, kc, S.font, false); setFont(lf); cx1 = (short)(cx1 + getTextWidth(kgS, -1) + gap); }   // (v0.52)
    textLJ(cx1, ly1, flS, C_FLIPC, S.font, false); setFont(lf); cx1 = (short)(cx1 + getTextWidth(flS, -1) + gap);
    textLJ(cx1, ly1, cwS, C_RES,   S.font, false); setFont(lf); cx1 = (short)(cx1 + getTextWidth(cwS, -1) + 8);
    if (has[1] && !lvlDepth[1].empty()) drawDepthPill(cx1, ly1, lvlDepth[1], S);
    (void)fIdx; (void)cIdx; sprintf_s(l1, sizeof(l1), "%s", "");
}
// Polarity legend (character, not strength). Placed opposite the panel's row.
void GammaProfile::drawLegend(RCT pane, const Settings& S)
{
    short lineH=(short)(S.font+6), pw=286, ph=(short)(2*lineH+14);
    short x=(short)(pane.left+8);
    // sit opposite the panel's row so they don't overlap
    short y = (S.panelpos < 3) ? (short)(pane.top+8) : (short)(pane.bottom-ph-8);
    RCT box; box.set(x, y, (short)(x+pw), (short)(y+ph));
    box.draw(1, C_GREY, C_DARK, DRAW_OPAQUE, PAT_SOLID);
    RCT sw;  sw.set((short)(x+10),(short)(y+9),(short)(x+22),(short)(y+9+11));
    sw.draw(0, S.cpos, S.cpos, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x+30),(short)(y+9+6),"+gamma gold: pins / fades", C_TXT, S.font-1, false);
    RCT sw2; sw2.set((short)(x+10),(short)(y+9+lineH),(short)(x+22),(short)(y+9+lineH+11));
    sw2.draw(0, S.cneg, S.cneg, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x+30),(short)(y+9+lineH+6),"-gamma magenta: breaks / wicks", C_TXT, S.font-1, false);
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
    // (v0.41) TAPE COLUMNS — a Skylit-ladder-style strip [SPX strike | %King] pinned to the pane edge on the
    // bar side. The bars are shifted inward by the strip width so the columns sit BESIDE them (the operator's
    // sketch: bars, then the values at the edge), and read row-for-row against Skylit's SPXW ladder.
    short colW = S.tapecols ? (short)(S.font * 9 + 8) : 0;
    short anchor;                 // base of bars
    int sgn;                      // +1 grows right, -1 grows left
    if (S.side == 1) {            // Left margin: anchor at pane left (+ column strip), grow right
        anchor = (short)(paneL + 2 + colW);
        sgn = +1;
    } else {                      // Right margin (default): anchor at pane right (− column strip), grow left
        sgn = -1;
        short edge = (short)(paneR - colW);
        if (S.detach) {
            anchor = edge;                        // fixed strip pinned to the right margin (inside the columns)
        } else {
            short a = (short)(lastX + w + 6);      // hug the candles (moves with them, fixed width)
            anchor = a < edge ? a : edge;
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

    // ---- structural roles: King / Ceiling / Floor / Gatekeeper (gpl::roles, unit-tested) ------
    // (v0.42) DOCTRINE: "the Gatekeeper" is ONE node, the dominant blocker strictly between spot and the
    // King (patternpedia/pattern-the-gatekeeper; the panel's gatekeeper() ranks by MAGNITUDE), >= 30% of
    // the King. v0.41 tagged every node >=10% on the path — the "G everywhere" flood. (v0.45) when the
    // gatekeeper is also the ceiling / floor the tag reads G·C / G·F, so the ceiling is never lost.
    std::vector<gpl::Node> gn(strikes.size());
    for (size_t i=0;i<strikes.size();i++){ gn[i].price=strikes[i].price; gn[i].pct=strikes[i].pct; gn[i].rank=strikes[i].rank;
                                           gn[i].king=strikes[i].king; gn[i].spx=strikes[i].spx; gn[i].type=strikes[i].type; }
    gpl::Roles RL = gpl::roles(gn, spotPx, hasSpot);
    float sp = hasSpot ? spotPx : ((RL.kIdx>=0) ? strikes[RL.kIdx].price : 0.0f);
    int kIdx=RL.kIdx, fIdx=RL.fIdx, cIdx=RL.cIdx;
    std::vector<int>& role = RL.role;

    // header (positionable: 0 TL,1 TC,2 TR,3 BL,4 BC,5 BR)
    if (S.header) {
        char h[96];
        int kingStrike = 0; for (size_t i=0;i<strikes.size();i++) if (strikes[i].king) kingStrike=(int)(strikes[i].price+0.5f);
        sprintf_s(h, sizeof(h), "%s gamma  King %d", (book == "IF0DTE") ? "IF 0DTE" : book.c_str(), kingStrike);
        FONT hf; hf.id=HELVETICA; hf.size=(short)S.font; hf.style=BOLD; setFont(hf);
        short tw = (short)getTextWidth(h, -1);
        int hp = S.headerpos;
        short hy = (hp<3) ? (short)(pane.top + S.font + 4) : (short)(pane.bottom - S.font - 4);
        short cx = (short)((paneL+paneR)/2 - tw/2);
        short hx = (hp%3==0) ? (short)(paneL+6) : (hp%3==1 ? cx : (short)(paneR - tw - 8));
        textLJ(hx, hy, h, C_TXT, S.font, true);
    }

    // spot marker
    if (S.spot && hasSpot) {
        PNT spm; spm.set(lastBar, spotPx);
        setPen(C_SPOT, 1, P_DOT);
        PNT a; a.set(0, spotPx); a.setDrawPosition();
        PNT b; b.set(lastBar, spotPx); b.drawLineTo();
    }

    // ---- air pockets: a thin run BETWEEN two real nodes = a fast pathway (gpl::airPockets) -----
    // DOCTRINE (learn/air-pockets-velocity): a low-exposure GAP between two significant nodes — trade THROUGH
    // it. Banded ONLY when a node >= 20% closes it on BOTH sides; the far-OTM tail is never shaded. Magenta
    // tint when the run's residual gamma is net negative (the violent version).
    if (S.roles) {
        std::vector<gpl::Pocket> pk = gpl::airPockets(gn);
        for (size_t q=0; q<pk.size(); q++) {
            bandPrice(pk[q].lo, pk[q].hi, paneL, paneR, pk[q].neg ? C_AIRN : C_AIR);
            PNT mid; mid.set(lastBar, (pk[q].lo + pk[q].hi)/2.0f);
            textLJ((short)(paneL+8), mid.v, pk[q].neg ? "AIR POCKET (-g)" : "AIR POCKET", C_GREY, S.font-1, false);
        }
    }

    // ---- deflection bands: the +/-5pt tap window around the top-5 nodes ------
    if (S.defbands) {
        for (size_t i=0;i<strikes.size();i++) if (strikes[i].rank>=1 && strikes[i].rank<=5) {
            COLOR c = strikes[i].pct>=0 ? S.cpos : S.cneg;
            PNT hi; hi.set(lastBar, strikes[i].price+5.0f);
            PNT lo; lo.set(lastBar, strikes[i].price-5.0f);
            hlinePx(hi.v, paneL, paneR, c, P_DOT);
            hlinePx(lo.v, paneL, paneR, c, P_DOT);
        }
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
        float t = ab / 100.0f; if (S.amp) t = t <= 0 ? 0 : (float)std::pow(t, 0.4f);   // amplify: steeper saturation
        COLOR col = (s.pct >= 0) ? lerpColor(S.cmid, S.cpos, t) : lerpColor(S.cmid, S.cneg, t);
        if (s.king && S.kingcol == 1) col = D_KING;
        if (!primary) col = C_GREY;

        // top-node line: a horizontal rail at each primary node, in the node's colour
        if (S.topnodes && primary) hlinePx(p.v, paneL, paneR, col, tps);

        short top = (short)(p.v - barH/2), bot = (short)(p.v + barH/2);
        short bl = sgn < 0 ? tip : anchor, br = sgn < 0 ? anchor : tip;
        drawBar(bl, top, br, bot, col, S.round, S.trans);

        // structural role: support/resistance side stripe at the base + role tag
        // (takes precedence over the plain node-type label)
        int rl = S.roles ? role[i] : 0;
        if (rl) {
            COLOR sr = (s.price < sp) ? C_SUP : C_RES;   // green support / red resistance
            short s1, s2; if (sgn<0){ s1=(short)(anchor-3); s2=anchor; } else { s1=anchor; s2=(short)(anchor+3); }
            RCT st; st.set(s1, (short)(p.v-barH/2-1), s2, (short)(p.v+barH/2+1));
            st.draw(0, sr, sr, DRAW_OPAQUE, PAT_SOLID);
            const char* rn = gpl::roleTag(rl, S.kinglabel);   // K(name) / C / F / G / G·C / G·F
            COLOR ic = inkOn(col);
            if (sgn<0) textRJ((short)(anchor-8), p.v, rn, ic, S.font, true);
            else       textLJ((short)(anchor+8), p.v, rn, ic, S.font, true);
        }

        // node PATTERN tag (P / B on the named stack member, R / RR on the rug's yellow node) INSIDE the bar.
        // (v0.42) drawn even when a role tag (C/F/G) sits at the base — a Rug ceiling is both "C" and "R".
        // (v0.45) placed just inside the TIP, beside the rank bubble, not centred: centred, the bubble sat on
        // top of it on the shorter bars (7600's tag was invisible under its "5"). Members carry no letter —
        // the bracket below marks the run (the operator chose the bracket over a letter on every member).
        const char* tlabel = s.king ? "" : gpl::patternTag(s.type);
        if (S.type && tlabel && tlabel[0] && len > (short)(S.font * 3)) {
            COLOR ic = inkOn(col);
            int rankMaxT = (S.filter <= 3) ? topN : (S.rankscope==1 ? 3 : 5);
            bool bubbleIn = S.rank && S.rankpos == 0 && s.rank >= 1 && s.rank <= rankMaxT;
            short r0 = (short)(S.font * 0.9f + 4);
            short inset = bubbleIn ? (short)(2 * r0 + 6) : 6;          // clear the bubble when it is inside the tip
            if (sgn < 0) textLJ((short)(tip + inset), p.v, tlabel, ic, S.font, true);
            else         textRJ((short)(tip - inset), p.v, tlabel, ic, S.font, true);
        }
        // (v0.42) LEVEL LABELS ON THE NODE: the wall nodes carry "CW" / "PW" just beyond the tip (outside the
        // rank bubble), in the wall colour, so the walls read off the histogram without a line across the chart.
        const char* wtag = 0;
        if (S.lvllabels) {
            if (has[1] && gpl::levelNode(gn, lvl[1], lvlSpx[1]) == (int)i) wtag = "CW";
            else if (has[2] && gpl::levelNode(gn, lvl[2], lvlSpx[2]) == (int)i) wtag = "PW";
        }
        short wtagW = 0, outerW = 0;   // (v0.51) outerW = tag + pill: what the outside % label must clear
        if (wtag) {
            FONT wf; wf.id = HELVETICA; wf.size = (short)(S.font - 1); wf.style = BOLD; setFont(wf);
            wtagW = (short)(getTextWidth(wtag, -1) + 6);
            int rankMaxW = (S.filter <= 3) ? topN : (S.rankscope==1 ? 3 : 5);
            bool bubbleOut = S.rank && S.rankpos == 1 && s.rank >= 1 && s.rank <= rankMaxW;
            short past = bubbleOut ? (short)(2 * (S.font * 0.9f + 4) + 8) : 6;
            if (sgn < 0) textRJ((short)(tip - past), p.v, wtag, C_PINK, S.font - 1, true);
            else         textLJ((short)(tip + past), p.v, wtag, C_PINK, S.font - 1, true);
            // (v0.50) the depth pill beside the tag (IF extras #1): 0D = today's expiry only, WK = the week, MO = the monthly
            // (v0.51) order outward from the tip: bubble, tag, pill, then the % label — the pill was drawn over the % (his
            // screenshot 21:33: "+48%" half under a WK box). outerW carries tag + pill so the % label clears both.
            outerW = (short)(past - 6 + wtagW);
            { const std::string& dtag = (wtag[0] == 'C') ? lvlDepth[1] : lvlDepth[2];
              if (!dtag.empty()) { short pw = pillW(dtag, S);
                                   if (sgn < 0) drawDepthPill((short)(tip - past - wtagW - 3 - pw), p.v, dtag, S);
                                   else         drawDepthPill((short)(tip + past + wtagW + 3), p.v, dtag, S);
                                   outerW = (short)(outerW + pw + 5); } }
        }

        // EM confluence: cyan tick at the tip when the node sits on an EM edge
        if (S.confl && ((has[4] && std::fabs(s.price-lvl[4])<=3.0f) || (has[5] && std::fabs(s.price-lvl[5])<=3.0f))) {
            setPen(C_CYAN, 2, P_SOLID);
            PNT a; a.set(0,0.0f); a.h=tip; a.v=(short)(p.v-6); a.setDrawPosition();
            PNT b; b.set(0,0.0f); b.h=tip; b.v=(short)(p.v+6); b.drawLineTo();
        }

        // RANK bubble -- scope follows the Show filter (Top3->3, Top5->5, ...);
        // for the non-TopN filters it falls back to the "Rank for" setting.
        int rankMax = (S.filter <= 3) ? topN : (S.rankscope==1 ? 3 : 5);
        bool inScope = (s.rank>=1 && s.rank<=rankMax);
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

        // % OUTSIDE the tip (or inside), signed — (v0.41) folded into the tape column when that is on
        if (S.showpct && !S.tapecols && ab >= (float)S.hideu) {
            char pc[12]; sprintf_s(pc, sizeof(pc), "%s%d%%", s.pct > 0 ? "+" : "", (int)(s.pct + (s.pct>=0?0.5f:-0.5f)));
            if (S.pctpos == 1) {  // inside near base
                COLOR ic = inkOn(col);
                if (sgn < 0) textLJ((short)(tip + 4), p.v, pc, ic, S.font, false);
                else         textRJ((short)(tip - 4), p.v, pc, ic, S.font, false);
            } else {              // outside the tip (shifted past a CW/PW tag when one is drawn there)
                if (sgn < 0) textRJ((short)(tip - 6 - outerW), p.v, pc, C_TXT, S.font, false);   // (v0.51) past tag + pill
                else         textLJ((short)(tip + 6 + outerW), p.v, pc, C_TXT, S.font, false);
            }
        }

        // (v0.41) TAPE COLUMNS: [SPX strike | %King] at the pane edge, on this node's row. The strike is the
        // RAW SPXW strike the panel carried (7575, 7580, ...) — the same digits Skylit's ladder prints — NOT the
        // ES-converted price, so the two ladders compare cell-for-cell. Greyed (sub-threshold) nodes print grey.
        if (S.tapecols) {
            char sk[16], pk[12];
            if (s.spx > 0.0f) sprintf_s(sk, sizeof(sk), "%d", (int)(s.spx + 0.5f)); else sk[0] = 0;
            sprintf_s(pk, sizeof(pk), "%s%d%%", s.pct > 0 ? "+" : "", (int)(s.pct + (s.pct>=0?0.5f:-0.5f)));
            COLOR tc = primary ? (s.king ? D_KING : C_TXT) : C_GREY;
            short half = (short)(colW / 2);
            if (S.side == 1) {        // left edge: strike column then % column, growing right
                short c1 = (short)(paneL + 4), c2 = (short)(paneL + 4 + half);
                if (sk[0]) textLJ(c1, p.v, sk, tc, S.font, s.king);
                textLJ(c2, p.v, pk, tc, S.font, s.king);
            } else {                  // right edge (default): strike column left, % column right-justified at the edge
                short c1 = (short)(paneR - colW + 4), c2 = (short)(paneR - 4);
                if (sk[0]) textLJ(c1, p.v, sk, tc, S.font, s.king);
                textRJ(c2, p.v, pk, tc, S.font, s.king);
            }
        }
    }

    // (v0.45) STACK BRACKETS — one thin bar along the base of the strip spanning each pika / barney run, in the
    // family's colour, so the stack reads as a SHAPE (its extent) with a single P / B on its biggest member.
    if (S.type) {
        std::vector<gpl::Stack> sk = gpl::stacks(gn);
        for (size_t q=0; q<sk.size(); q++) {
            if (sk[q].n < 2) continue;
            PNT a; a.set(lastBar, sk[q].lo); PNT b; b.set(lastBar, sk[q].hi);
            short y1 = (short)((a.v < b.v ? a.v : b.v) - barH/2), y2 = (short)((a.v < b.v ? b.v : a.v) + barH/2);
            short bx = (sgn < 0) ? (short)(anchor + 2) : (short)(anchor - 5);
            RCT br; br.set(bx, y1, (short)(bx + 3), y2);
            COLOR bc = sk[q].pos ? S.cpos : S.cneg;
            br.draw(0, bc, bc, DRAW_OPAQUE, PAT_SOLID);
        }
    }

    // (v0.42) FLIP tick — the zero-gamma level is a PRICE, not a strike, so it cannot tag a node: a short dashed
    // tick across the bar strip at that price, labelled FLIP (+ window), in the flip colour. Independent of the line.
    if (S.lvllabels && has[3]) {
        PNT fp; fp.set(lastBar, lvl[3]);
        short x1 = (sgn < 0) ? (short)(anchor - w) : anchor, x2 = (sgn < 0) ? anchor : (short)(anchor + w);
        hlinePx(fp.v, x1, x2, C_FLIPC, P_DASH);
        char fl[24]; sprintf_s(fl, sizeof(fl), "FLIP%s%s", lvlWin[3].empty() ? "" : " ", lvlWin[3].c_str());
        if (sgn < 0) textRJ((short)(anchor - 8), (short)(fp.v - S.font/2 - 1), fl, C_FLIPC, S.font - 1, true);
        else         textLJ((short)(anchor + 8), (short)(fp.v - S.font/2 - 1), fl, C_FLIPC, S.font - 1, true);
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

    // regime + read panel (net signed gamma -> fade vs follow), then legend
    if (S.regime) {
        float net = 0.0f; for (size_t i=0;i<strikes.size();i++) net += strikes[i].pct;
        drawPanel(pane, S, net, fIdx, cIdx, kIdx);
    }
    if (S.legend) drawLegend(pane, S);
}

// ---- draw() ---------------------------------------------------------------
int GammaProfile::draw(void)
{
    load();
    applyContractOffset();  // (v0.39) align to this chart's contract BEFORE render maps prices to Y
    render(cfg);       // use the cached settings (read in parms callbacks, valid even when the dialog is closed)
    drawStaleBadge();  // (v0.38) warn if the CSV is cold, regardless of what render drew
    return RTX_OK;
}

// ---- (v0.39) CONTRACT ALIGNMENT — the whole book is priced in the panel's space (Skylit ES1 / cash,
// SPOT is the anchor). This chart may be a DIFFERENT contract (e.g. EPZ26 December, ~+70 over cash from
// carry), so everything would draw off the bottom. Shift every price by (this chart's own last close −
// SPOT): that anchors SPOT to the chart's live price and carries the whole book with it, so the King,
// nodes and levels sit where they belong regardless of which contract is charted. It self-corrects each
// redraw as the spread moves. Clamped so a wrong chart / bad SPOT can never fling the book to nonsense.
void GammaProfile::applyContractOffset()
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
    // (v0.40) ANCHOR ON SCALEREF, NOT SPOT. The ladder (King, nodes, walls) is priced in the panel's
    // front-month scale (Skylit ES1, via esOfSpx). SPOT is the day-model's own value, and during the
    // quarterly roll it is already ~this chart's (Dec) scale — so anchoring on it made off≈0 and left the
    // front-scale ladder ~one calendar-spread (~60-90 pts) below price. SCALEREF is the front price the
    // ladder IS in, so off = chartClose − SCALEREF is exactly that spread; shifting by it lands the whole
    // book on the charted contract, and →0 on its own once the front rolls to this contract. Fall back to
    // SPOT only when SCALEREF is absent (older panel, or a book that doesn't write it).
    float off = 0.0f;
    if (!gpl::contractOffset(chartClose, hasScaleRef, scaleRef, hasSpot, spotPx, off)) return;   // no anchor / implausible → leave as-is
    for (size_t i = 0; i < strikes.size(); i++) strikes[i].price += off;
    for (int i = 0; i < 6; i++) if (has[i]) lvl[i] += off;
    if (hasSpyKing) spyKingPx += off;
    // The spot marker AND the role (King/Ceiling/Floor/Gatekeeper) test want the chart's LIVE price, not the
    // shifted CSV spot — so pin spot to the chart close. This is also what breaks the "everything is a
    // Gatekeeper" flood: with spot and King finally on the SAME (chart) scale, only the nodes genuinely
    // between price and the King fall inside the gate band.
    spotPx = chartClose; hasSpot = true;
}

// ---- (v0.38) STALE badge — shared logic across all four plugins ------------
// The panel stamps each export with ASOF,<CT sec-of-day>. Compare to the chart clock (assumed CT, same
// as the King tracker); if the data is older than ~4 min (the panel writes every ~3 min), a frozen file
// is drawing an old book — flag it so it can never be mistaken for live. Handles the overnight wrap.
void GammaProfile::drawStaleBadge()
{
    if (asofSo < 0) return;
    RTDATE now = currentDate(); struct tm tmv; memset(&tmv, 0, sizeof(tmv)); getLocaltime(now, &tmv);
    double localSo = tmv.tm_hour * 3600.0 + tmv.tm_min * 60.0 + tmv.tm_sec;
    double ageMin = (asofSo > localSo + 300.0) ? ((86400.0 - asofSo) + localSo) / 60.0 : (localSo - asofSo) / 60.0;
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
    GammaProfile *p = new GammaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("Gamma node profile + level rail (reads lsFlexLevels\\GammaProfile.csv)");
    p->setVersion("0.56");
    return p;
}
