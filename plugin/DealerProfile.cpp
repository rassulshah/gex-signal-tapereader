/********************************************************************************
 *  DealerProfile.cpp  --  Investor/RT RTX extension  lsDealerProfile  (v1.0, 2026-09-29)
 *
 *  THE DEALER PROFILE: two nodes per strike on the right edge of the price pane, facing price.
 *     (1.3.6: delta on TOP, gamma below)
 *     lower node  synthetic GAMMA on arrival (futures per 0.1 EM): cyan = dealers long gamma (a WALL), lime = short (FUEL)
 *                 "4220  10  (9 to 17)": 10 at the usual pace, 9 if price gets there twice as fast, 17 twice as slow
 *                 (1.3.5 - the whisker line is gone, Rassul 2026-09-30); outline = MenthorQ's snapshot now
 *     upper node  synthetic DELTA: futures dealers must trade because of this strike on the way there (green BUY / red SELL)
 *  Values sit INSIDE the nodes when they fit (operator 2026-09-29: no text outside the nodes); the rest is on the
 *  whole-book banner (top-left) and the legend line. Strikes beyond 1.2 EM fade.
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv, written every 5 min by the LRA Reader
 *  (level-reversal-analytics analytics/lra/dealer_irt.py). Grammar and decisions: DealerLogic.h (tested,
 *  test_dealer_logic.cpp). The market comes from the chart's root symbol (EP -> ES, GCE -> GC ...) unless set.
 *
 *  A separate indicator from lsDealerRead (the checklist strip), so each can be on or off, on any chart.
 *  Parameters are read ONLY in the parms callbacks (GAMMA-PROFILE-PLUGIN.md gotcha 3); positions numbered explicitly.
 *  Never black: the chart background is black.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

static const char* DP_VERSION = "1.3.6";   // 1.3.4: cyan wall / lime fuel. 1.3.5: "4220  10  (9 to 17)", no whisker; delta keeps BUY / SELL
static const COLOR C_WALL  = 0x0022D3EE;   // long gamma: CYAN (1.3.4, Rassul 2026-09-30: "Cyan (wall) and Lime (fuel)")
static const COLOR C_FUEL  = 0x00A3E635;   // short gamma: LIME (delta keeps green buy / red sell)
static const COLOR C_BUY   = 0x0022C55E;   // dealers buy
static const COLOR C_SELL  = 0x00EF4444;   // dealers sell
static const COLOR C_WHISK = 0x00F8FAFC;   // arrival range
static const COLOR C_SNAP  = 0x00E5E7EB;   // snapshot outline
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;   // fills and text ON a coloured node only (never a line)
static const COLOR C_BANNERBG = 0x001C1305;

static COLOR fade(COLOR a, float t)   // toward the dark ground
{
    int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    int br = (C_DARK >> 16) & 0xFF, bg = (C_DARK >> 8) & 0xFF, bb = C_DARK & 0xFF;
    ar = (int)(ar + (br - ar) * t); ag = (int)(ag + (bg - ag) * t); ab = (int)(ab + (bb - ab) * t);
    return (COLOR)(((COLOR)ar << 16) | ((COLOR)ag << 8) | (COLOR)ab);
}

struct PIdx { int market, width, labels, banner, legend, snap, whisk, fadefar, font, clock, keyPill, keyNode; };
static PIdx PX;
struct Settings { int market = 0, width = 260, font = 9, clock = 0; bool labels = true, banner = true, legend = true, snap = true, whisk = true, fadefar = true; };

class DealerProfile : public cppExtension {
public:
    DealerProfile();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    Settings cfg;
    dl::Data D;
    std::string mkt, root;
    float off;
    int lastBar;

    bool dialogReady();
    void readSettings(Settings& S);
    void load();
    void alignContract();
    void render(const Settings& S);
    void staleBadge();
    void writeStatus(const char* what);
    short yOf(float price);
    void box(short l, short t, short r, short b, COLOR c);
    void dashRect(short l, short t, short r, short b, COLOR c);
    void line(short x1, short y1, short x2, short y2, COLOR c, int w);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold);
    int  textW(const char* s, int sz, bool bold);
    void strengthMark(short rightX, short y, float pct, bool live, int font);
    void outline(short l, short t, short r, short b);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerProfile::DealerProfile() : cppExtension() { off = 0.0f; lastBar = 0; }

bool DealerProfile::dialogReady() { int i = getListIndex(PX.market); return i >= 0 && i <= 7; }
int DealerProfile::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerProfile::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerProfile::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(17);   // (1.3.5) room for the how-to guide
    const short SL = kParmAppendSameLine;
    int pc = 0;
    // IRT keeps a saved instance's values BY POSITION: append new rows at the end, never reorder
    PX.market  = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    PX.width   = pc++; setIntegerParameter("Profile width px", 260, 0, SL);
    PX.labels  = pc++; setBoolParameter("Values inside the nodes", true);
    PX.snap    = pc++; setBoolParameter("Snapshot outline", true, SL);
    PX.whisk   = pc++; setBoolParameter("(whisker removed - the range is in the text)", false);   // 1.3.5: kept for its position, unused
    PX.fadefar = pc++; setBoolParameter("Fade strikes beyond 1.2 EM", true, SL);
    PX.banner  = pc++; setBoolParameter("Whole-book banner", true);
    PX.legend  = pc++; setBoolParameter("(key moved to the description above)", false, SL);
    PX.font    = pc++; setIntegerParameter("Font size (pt)", 9, 0);
    PX.clock   = pc++; setIntegerParameter("Clock offset (min)", 0, 0, SL);
    // (1.3.3) HOW TO READ IT, in the settings (the description box cuts long text off): open a list to read every line.
    // Appended LAST so saved values keep their positions; nothing reads them.
    PX.keyPill = pc++; setListParameter("Pill (open to read)", 0, "The pill = the node's size vs a normal 15 min of futures volume;Grey text = under 0.25%: quiet - ignore it;Dark pill = 0.25-1%: small - context only;Yellow pill = 1-3%: meaningful;WHITE pill + white outline = 3%+: MARKET-MOVING;Bolt = dealers ARE trading there now (COVER 5%+ after the sweep);Bands = 9/25 first look - the nightly study will set them per market");
    PX.keyNode = pc++; setListParameter("Nodes (open to read)", 0, "Read each strike top to bottom: DELTA then GAMMA;Upper node = DELTA: futures dealers must trade on the way there;Green = dealers buy / red = dealers sell;Lower node = GAMMA when price gets there: futures per 0.1 EM (the points are in the top banner);Cyan = WALL: dealers lean against every tick - stops a sweep;Lime = FUEL: dealers chase the move - then unwind it after the turn;(9 to 17) = if price gets there twice as fast / twice as slow;Thin outline = MenthorQ's number right now;Example 9/29 Gold: 4215  32 (28 to 46) wall with SELL 81 on the way = the high");
    // (1.3.5, Rassul 2026-09-30: "keep an example of the read in the settings - a How-to guide ... use all the blank space")
    // one full row per line (a checkbox label shows the whole line); appended LAST so saved values keep their positions;
    // nothing reads them
    pc++; setBoolParameter("HOW TO READ A STRIKE - 9/29 Gold 4215 at 08:45 (the high)", false);
    pc++; setBoolParameter("1 WHERE: price 23 pts under 4215, heading up", false);
    pc++; setBoolParameter("2 DELTA (upper node): SELL 81 = dealers sell 81 on the way", false);
    pc++; setBoolParameter("   up -> against the rally: the push fades before the level", false);
    pc++; setBoolParameter("3 GAMMA (lower node): 4215  32  (28 to 46), CYAN = a WALL", false);
    pc++; setBoolParameter("   every 5.5 pts higher, dealers sell 32 more futures", false);
    pc++; setBoolParameter("   46 if price grinds up slowly, 28 if it spikes in", false);
    pc++; setBoolParameter("4 SIZE: yellow pill 2.8% of a normal 15 min = meaningful", false);
    pc++; setBoolParameter("5 VERDICT: headwind + wall = the high holds. Wait for the", false);
    pc++; setBoolParameter("   sweep + a trigger, then short. (It held.)", false);
    pc++; setBoolParameter("LIME = FUEL: dealers chase the move -> overshoot, snap back", false);
    pc++; setBoolParameter("   12:57: SELL 222 into 4175 (lime) -> swept, then dealers", false);
    pc++; setBoolParameter("   bought 700 / 606 / 931 back = the long", false);
    return RTX_OK;
}

void DealerProfile::readSettings(Settings& S)
{
    S.market = getListIndex(PX.market);
    S.width = getIntegerValue(PX.width); if (S.width < 40) S.width = 260; if (S.width > 900) S.width = 900;   // (1.0.1) a first dialog showed 9: anything under 40 px is not a real width
    S.labels = isBoxChecked(PX.labels) != 0; S.snap = isBoxChecked(PX.snap) != 0; S.whisk = isBoxChecked(PX.whisk) != 0;
    S.fadefar = isBoxChecked(PX.fadefar) != 0; S.banner = isBoxChecked(PX.banner) != 0; S.legend = isBoxChecked(PX.legend) != 0;
    S.font = getIntegerValue(PX.font); if (S.font < 6) S.font = 9; if (S.font > 24) S.font = 24;
    S.clock = getIntegerValue(PX.clock); if (S.clock < -720) S.clock = -720; if (S.clock > 720) S.clock = 720;
}

void DealerProfile::load()
{
    D = dl::Data();
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    std::ifstream f(path.c_str()); if (!f.is_open()) return;
    std::stringstream ss; ss << f.rdbuf();
    D = dl::parseText(ss.str());
}

// the chart may be another contract than MenthorQ's front future: shift by (close at the file's minute - PRICE)
void DealerProfile::alignContract()
{
    off = 0.0f;
    long n = getBarCount(); if (n < 1 || !D.hasPrice) return;
    RTARRAY close(barClose);
    RTARRAYI dt(barDateTime);
    float c = close[(int)n - 1];
    if (D.asofSo >= 0) {
        double want = D.asofSo + cfg.clock * 60.0;
        int from = (int)n - 3000; if (from < 0) from = 0;
        for (int i = (int)n - 1; i >= from; i--) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
            double so = t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec;
            bool sameDay = (D.y == 0) || (t.tm_year + 1900 == D.y && t.tm_mon + 1 == D.mo && t.tm_mday == D.d);
            if (sameDay && so <= want) { c = close[i]; break; }
        }
    }
    float o = 0; if (dl::offsetFor(c, D.px, o)) off = o;
}

short DealerProfile::yOf(float price) { PNT p; p.set(lastBar, price); return p.v; }
void DealerProfile::box(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
void DealerProfile::line(short x1, short y1, short x2, short y2, COLOR c, int w)
{
    setPen(c, (short)w, P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
}
void DealerProfile::dashRect(short l, short t, short r, short b, COLOR c)
{
    setPen(c, 1, P_SOLID);   // (1.3.2) operator: "the border of the node is dashed, make it solid"
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
int DealerProfile::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s, -1);
}
// the rect text draw baselines low (~0.45 x font below y, GammaProfile v0.73): shift up so it sits ON y
void DealerProfile::textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
    RCT rc; rc.set((short)(rightX - 400), (short)(yy - sz), rightX, (short)(yy + sz));
    rc.drawText(s, false, true);
}
void DealerProfile::textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
    RCT rc; rc.set(leftX, (short)(yy - sz), (short)(leftX + 900), (short)(yy + sz));
    rc.drawText(s, false, false);
}

void DealerProfile::render(const Settings& S)
{
    long n = getBarCount(); if (n < 2) return;
    lastBar = (int)n - 1;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    short anchor = (short)(paneR - 2);
    int W = S.width;

    if (!D.hasPrice || D.nodes.empty()) {
        char b[96]; sprintf_s(b, sizeof(b), "Dealer Profile: no data for %s (root %s)", mkt.empty() ? "?" : mkt.c_str(), root.c_str());
        textLJ((short)(pane.left + 8), (short)(pane.top + 14), b, C_MUTED, S.font, false);
        return;
    }
    float gmax = 0, dmax = 0;
    {   // (1.3.1) scale to the nodes ON SCREEN: a big strike off the visible price range (9/29 night: 4,150 at -47) shrank every
        // node in view to a few pixels
        dl::Data V = D; V.nodes.clear();
        for (size_t i = 0; i < D.nodes.size(); i++) {
            if (D.nodes[i].far) continue;
            short yy = yOf(D.nodes[i].k + off);
            if (yy >= pane.top && yy <= pane.bottom) V.nodes.push_back(D.nodes[i]);
        }
        dl::scales(V.nodes.empty() ? D : V, gmax, dmax);
    }
    // strike spacing in pixels
    float step = 0;
    for (size_t i = 1; i < D.nodes.size(); i++) { float d = D.nodes[i].k - D.nodes[i - 1].k; if (d > 0 && (step == 0 || d < step)) step = d; }
    if (step <= 0) step = D.em * 0.1f;
    int hs = std::abs((int)yOf(D.nodes[0].k + off) - (int)yOf(D.nodes[0].k + step + off));
    if (hs < 6) hs = 6;
    int nodeH = (int)(hs * 0.42f); if (nodeH < 3) nodeH = 3;
    int gap = hs >= 12 ? 1 : 0;

    for (size_t i = 0; i < D.nodes.size(); i++) {
        const dl::Node& N = D.nodes[i];
        short y = yOf(N.k + off);
        if (y < pane.top - hs || y > pane.bottom + hs) continue;
        float ft = (S.fadefar && N.far) ? 0.65f : 0.0f;
        int Lg = dl::barLen(N.g, gmax, W), Ls = dl::barLen(N.snap, gmax, W), Ld = dl::barLen(N.d, dmax, W);
        // (1.3.6, Rassul 2026-09-30: "delta on top because that is how we read it") DELTA on top (what dealers trade on the
        // way there), GAMMA below (what happens at the level) - read each strike top to bottom
        short dt_ = (short)(y - gap - nodeH), db = (short)(y - gap), gt = (short)(y + gap), gb = (short)(y + gap + nodeH);
        COLOR gc = fade(N.g >= 0 ? C_WALL : C_FUEL, ft), dc = fade(N.d >= 0 ? C_BUY : C_SELL, ft);
        if (Lg > 0) box((short)(anchor - Lg), gt, anchor, gb, gc);
        if (Lg > 2 && dl::strengthBars(N.gp) == 3) outline((short)(anchor - Lg), gt, anchor, gb);   // (1.3) market-moving: 3%+
        if (S.snap && Ls > 1) dashRect((short)(anchor - Ls), gt, anchor, gb, fade(C_SNAP, ft));
        if (Ld > 0) box((short)(anchor - Ld), dt_, anchor, db, dc);
        if (Ld > 2 && dl::strengthBars(N.dp) == 3) outline((short)(anchor - Ld), dt_, anchor, db);
        if (false && S.whisk && !N.far && std::fabs(N.hi - N.lo) >= 1.0f) {   // (1.3.5) no whisker: "(9 to 17)" is in the node's text
            int a = dl::barLen(N.lo, gmax, W), b = dl::barLen(N.hi, gmax, W);
            short ym = (short)((gt + gb) / 2), cap = (short)(nodeH / 2 > 2 ? nodeH / 2 : 2);
            short x1 = (short)(anchor - (a > b ? a : b)), x2 = (short)(anchor - (a < b ? a : b));
            line(x1, ym, x2, ym, C_WHISK, 1);
            line(x1, (short)(ym - cap), x1, (short)(ym + cap), C_WHISK, 1);
            line(x2, (short)(ym - cap), x2, (short)(ym + cap), C_WHISK, 1);
        }
        if (S.labels && !N.far && nodeH >= S.font - 1) {
            std::string gl = dl::gammaLabel(N), dlab = dl::deltaLabel(N);
            if (!dl::fits(textW(gl.c_str(), S.font, true), Lg)) gl = dl::gammaShort(N);   // (1.3.5) short node: "4220  10"
            if (dl::fits(textW(gl.c_str(), S.font, true), Lg)) textRJ((short)(anchor - 4), (short)((gt + gb) / 2), gl.c_str(), C_DARK, S.font, true);
            if (dl::fits(textW(dlab.c_str(), S.font, true), Ld)) textRJ((short)(anchor - 4), (short)((dt_ + db) / 2), dlab.c_str(), N.d < 0 ? 0x00FFFFFF : C_DARK, S.font, true);
        }
        if (S.labels && !N.far && nodeH >= S.font - 3) {   // (1.2) the strength share right OUTSIDE each node: % of a normal 15 min of volume
            int reachG = Lg; if (S.snap && Ls > reachG) reachG = Ls;
            if (false && S.whisk) { int a2 = dl::barLen(N.lo, gmax, W), b2 = dl::barLen(N.hi, gmax, W); if (a2 > reachG) reachG = a2; if (b2 > reachG) reachG = b2; }
            // (1.2) [lightning if LIVE] [strength bars] [%]  right outside the node, coloured by band (Rassul 2026-09-29)
            if (Lg > 0 && N.gp >= 0) strengthMark((short)(anchor - reachG - 4), (short)((gt + gb) / 2), N.gp, N.live, S.font);
            if (Ld > 0 && N.dp >= 0) strengthMark((short)(anchor - Ld - 4), (short)((dt_ + db) / 2), N.dp, false, S.font);
        }
    }
    // whole-book banner + legend (top-left of the pane)
    short bx = (short)(pane.left + 8), by = (short)(pane.top + 6);
    if (S.banner && !D.bookLine.empty()) {
        int tw = textW(D.bookLine.c_str(), S.font + 1, true);
        RCT bg; bg.set(bx, by, (short)(bx + tw + 20), (short)(by + S.font + 12));
        bg.draw(1, C_FUEL, C_BANNERBG, DRAW_OPAQUE, PAT_SOLID);
        textLJ((short)(bx + 10), (short)(by + (S.font + 12) / 2), D.bookLine.c_str(), D.book < 0 ? C_FUEL : C_WALL, S.font + 1, true);
        by = (short)(by + S.font + 16);
    }
    if (false && S.legend) {   // (1.1.1) operator 2026-09-29: "no more text on the chart - add it to the indicator" (the description)       // (1.1) operator 2026-09-29: say what each node is FOR, not only what it is
        short ly = (short)(by + S.font / 2 + 2), lh = (short)(S.font + 5);
        textLJ((short)(bx + 2), ly, "GAMMA node (lower): how hard this strike pushes back WHEN PRICE GETS THERE.  Cyan = WALL, dealers lean against every tick - stops a sweep and turns it.  Lime = FUEL, dealers chase the move there.", C_INK, S.font - 1, false);
        textLJ((short)(bx + 2), (short)(ly + lh), "DELTA node (upper): futures dealers MUST trade on the way to this strike (green buy / red sell) - the push into your level, and the stock they unwind after the turn (fuel for the reversal).", C_INK, S.font - 1, false);
        textLJ((short)(bx + 2), (short)(ly + 2 * lh), "Use both: the delta node says if there is flow to push price into the level; the gamma node says if the level stops it.  Whisker = earlier/later arrival, dashed = MenthorQ's number now.", C_MUTED, S.font - 1, false);
    }
}

// (1.2) the strength mark, drawn right-to-left ending at rightX: "2.8%" in its band colour, then 0-3 rising bars, then a
// lightning bolt when COVER has confirmed dealers are actually trading at this level. Shapes, not glyphs (IRT text is ASCII).
void DealerProfile::outline(short l, short t, short r, short b)
{
    setPen(0x00FFFFFF, 2, P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
static COLOR bandCol(int b) { return b == 0 ? 0x006B7280 : b == 1 ? 0x00E5E7EB : b == 2 ? 0x00FCD34D : 0x00FFFFFF; }
void DealerProfile::strengthMark(short rightX, short y, float pct, bool live, int font)
{
    // (1.3) Rassul 2026-09-29: "pill and the outline for market moving nodes" - the % sits in a rounded pill coloured by
    // its band (quiet = grey text only, small = dark pill, meaningful = yellow pill, big = white pill); the bolt goes left
    int b = dl::strengthBars(pct);
    std::string t = dl::pctTxt(pct);
    int fsz = font - 1; if (fsz < 6) fsz = 6;
    int tw = textW(t.c_str(), fsz, b >= 1);
    short x = rightX;
    if (b == 0) {
        textRJ(rightX, y, t.c_str(), 0x006B7280, fsz, false);
        x = (short)(rightX - tw - 4);
    } else {
        COLOR fill = b == 1 ? 0x00334155 : (b == 2 ? 0x00FCD34D : 0x00FFFFFF);   // (1.3.1) dark pill lighter: visible on black
        COLOR ink  = b == 1 ? 0x00F1F5F9 : 0x000B0F19;
        short padx = 6, h = (short)(fsz + 6);
        short w = (short)(tw + 2 * padx + 2);                     // (1.3.3) + the bold overrun, so both sides match
        short l = (short)(rightX - w), t0 = (short)(y - h / 2), bt = (short)(t0 + h);
        setPen(fill, 1, P_SOLID);
        CBRUSH br(fill, PAT_SOLID); br.set();
        RCT rc; rc.set(l, t0, rightX, bt); rc.drawRounded(6, 6);   // (1.3.1) a pill, not an oval (the corner was h - 2)
        {   // (1.3.2) centre the text on the pill by its measured metrics (GammaProfile textC) - the rect draw left uneven sides
            FONT f; f.id = HELVETICA; f.size = (short)fsz; f.style = BOLD; setFont(f);
            int lead = 0, asc = 0, desc = 0; getFontMetrics(&lead, &asc, &desc);
            short twc = (short)getTextWidth(t.c_str(), -1);
            setTextColor(ink);
            PNT tp; tp.set(0, 0.0f); tp.h = (short)(l + (w - twc) / 2); tp.v = (short)(y + (asc - desc) / 2); tp.drawText(t.c_str());
        }
        x = (short)(l - 4);
    }
    if (live) {                                              // the yellow lightning bolt: COVER confirmed dealers trading here
        short h = (short)(fsz + 2), w = (short)(h / 2 + 1);
        short top = (short)(y - h / 2), bot = (short)(y + h / 2), l = (short)(x - w), r = x;
        COLOR c = 0x00FACC15;
        line(r, top, (short)(l + 1), (short)(y + 1), c, 2);
        line((short)(l + 1), (short)(y + 1), (short)(r - 1), (short)(y - 1), c, 2);
        line((short)(r - 1), (short)(y - 1), l, bot, c, 2);
    }
}

void DealerProfile::staleBadge()
{
    if (D.asofSo < 0) return;
    RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t);
    double age = dl::staleMin(D.asofSo + cfg.clock * 60.0, t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec);
    if (age <= 10.0) return;
    char b[40]; if (age >= 90) sprintf_s(b, sizeof(b), "DEALER DATA STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "DEALER DATA STALE %dm", (int)(age + 0.5));
    RCT pane; pane.getPaneRect(false);
    int tw = textW(b, 10, true);
    short x = (short)(pane.right - tw - 90), y = (short)(pane.top + 6);
    RCT bg; bg.set(x, y, (short)(x + tw + 14), (short)(y + 18));
    bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x + 7), (short)(y + 9), b, 0x00FF9A8F, 10, true);
}

void DealerProfile::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION," << DP_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nNODES," << D.nodes.size() << "\nOFFSET," << off << "\nSTATE," << what << "\n";
}

int DealerProfile::draw(void)
{
    load();
    alignContract();
    render(cfg);
    staleBadge();
    writeStatus(D.nodes.empty() ? "no data" : "drawn");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerProfile *p = new DealerProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Dealer Profile. Read each strike top to bottom: upper node = futures dealers must trade on the way (delta), lower node = gamma when price gets there (cyan wall stops a sweep, lime fuel chases it). Pill = size vs a normal 15 min of volume; white pill + outline = market-moving (3%+). The HOW TO READ guide is below; open the two lists for every detail.");
    p->setVersion("1.3.6");
    return p;
}
