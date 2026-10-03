/********************************************************************************
 *  DealerProfile.cpp  --  Investor/RT RTX extension  lsDealerProfile  (v1.0, 2026-09-29)
 *
 *  2.0.0 (2026-10-02): THE DEALER PROFILE = one bar per strike of MenthorQ's net GEX (dealer delta change in $ per 1-pt
 *     move): red = dealers SHORT gamma (fuel: they chase price), green = LONG (wall: they lean against it), the darker part
 *     = today's 0DTE (gone at expiry), the value ("-3.9M") on the bar. Same numbers as MenthorQ's Net GEX panel. The running
 *     BUY / SELL total moved to the Dealer Read. Files without net GEX (older Reader) still draw the 1.x nodes below.
 *  1.x: two nodes per strike on the right edge of the price pane, facing price.
 *     (1.3.6: delta on TOP, gamma below)
 *     lower node  synthetic GAMMA on arrival (futures per 0.1 EM): yellow = dealers long gamma (a WALL), purple = short (FUEL)
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

static const char* DP_VERSION = "2.1.0";   // 2.1.0 (2026-10-03): SPX / QQQ book tag (file 2.0 SRC row)
//   // 2.0.3 (2026-10-03): number boxes get an explicit width (NUMW) - 0 is the SDK default and still showed "T"
//   // 2.0.2 (2026-10-03, Rassul: "ng looks wierd", "euro also looks strange", font / clock show T): bars capped in height (NG / EU strikes are 100-300 px apart when zoomed in - each bar was a block), values on every visible bar, CL / NG dimmed (options data context only), stale age by date, number fields at the default width
//   // 2.0.1 (2026-10-02 19:12): the how-to text in the settings is gone   // 2.0.0 (2026-10-02, Rassul: "there is no way it is supposed to look like this"): one bar per strike = MenthorQ's net GEX ($ per 1-pt move), red short / green long, today's 0DTE part darker, the value on the bar - as MenthorQ draws it; the old synthetic gamma + running BUY/SELL nodes only for files without net GEX   // 1.5.0: option D pills - the % pill takes its node's colour, filled by size (HEAVY solid / BIG half / MODERATE faint / LIGHT outline), as the Reader's size chips   // 1.4.0: KEY / TGT / MAG tags, REACH in the status (mockup v26)   // 1.3.11: settings guide = (open) lists, no checkbox rows   // 1.3.10: whole-book banner off by default (Rassul 2026-09-30: "i dont think i need the top part")
//   // 1.3.4: cyan wall / lime fuel. 1.3.5: "4220  10  (9 to 17)", no whisker; delta keeps BUY / SELL
static const COLOR C_WALL  = 0x00E3C341;   // (1.3.9, Rassul 2026-09-30: "yellow and purple like Skylit") long gamma = YELLOW //   // long gamma: CYAN (1.3.4, Rassul 2026-09-30: "Cyan (wall) and Lime (fuel)")
static const COLOR C_FUEL  = 0x00AB47BC;   // short gamma = PURPLE (Skylit) //   // short gamma: LIME (delta keeps green buy / red sell)
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

struct PIdx { int market, width, labels, banner, legend, snap, whisk, fadefar, font, clock, keyPill, keyNode, keyHow, inds; };
static PIdx PX;
struct Settings { int market = 0, width = 260, font = 9, clock = 0; bool labels = true, banner = false, legend = true, snap = true, whisk = true, fadefar = true;
                  std::string inds = "cob_bull,cob_bear"; };   // (1.5.0) his custom indicators copied into the bar file

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
    long long loadedStamp = -2; std::string loadedPath;   // (1.3.7) re-read the file only when it changed
    long expN = 0; float expC = 0; time_t expT = 0;          // (1.3.7) the chart-bar export
    void exportBars();

    bool dialogReady();
    void readSettings(Settings& S);
    void load();
    void alignContract();
    void render(const Settings& S);
    void staleBadge();
    void srcTag();
    void writeStatus(const char* what);
    short yOf(float price);
    void box(short l, short t, short r, short b, COLOR c);
    void dashRect(short l, short t, short r, short b, COLOR c);
    void line(short x1, short y1, short x2, short y2, COLOR c, int w);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold);
    int  textW(const char* s, int sz, bool bold);
    void strengthMark(short rightX, short y, float pct, bool live, int font, COLOR base);
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
    // (1.3.12, Rassul 2026-09-30) only real settings have a control; the guide is plain text (setLabelParameter) below.
    // IRT keeps saved values BY POSITION (the parameter version does not reset them): remove + re-add the indicator once.
    setParameterVersion(3);
    setParameterDialogHeight(8);    // (2.0.1, Rassul 2026-10-02 19:12) the how-to text is gone - controls only
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.market  = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    // (2.0.3) NUMW (DealerLogic.h): the SDK default width (0) made the number boxes so narrow they showed "T"
    PX.width   = pc++; setIntegerParameter("Profile width px", 260, NUMW, SL);
    PX.font    = pc++; setIntegerParameter("Font size (pt)", 9, NUMW);
    PX.clock   = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SL);
    PX.labels  = pc++; setBoolParameter("Values on the bars", true);
    PX.snap    = pc++; setBoolParameter("Snapshot outline (1.x files)", true, SL);
    PX.fadefar = pc++; setBoolParameter("Fade strikes beyond 1.2 EM", true);
    PX.banner  = pc++; setBoolParameter("Whole-book banner", false, SL);
    PX.inds    = pc++; setStringParameter("Your indicators in the bar file (names, comma)", "cob_bull,cob_bear", 160);
    PX.keyHow = PX.keyNode = PX.keyPill = PX.whisk = PX.legend = -1;
    return RTX_OK;
}

void DealerProfile::readSettings(Settings& S)
{
    S.market = getListIndex(PX.market);
    S.width = getIntegerValue(PX.width); if (S.width < 40) S.width = 260; if (S.width > 900) S.width = 900;   // (1.0.1) a first dialog showed 9: anything under 40 px is not a real width
    S.labels = isBoxChecked(PX.labels) != 0; S.snap = isBoxChecked(PX.snap) != 0; S.whisk = false;
    S.fadefar = isBoxChecked(PX.fadefar) != 0; S.banner = isBoxChecked(PX.banner) != 0; S.legend = false;
    S.font = getIntegerValue(PX.font); if (S.font < 6) S.font = 9; if (S.font > 24) S.font = 24;
    S.clock = getIntegerValue(PX.clock); if (S.clock < -720) S.clock = -720; if (S.clock > 720) S.clock = 720;
    char ib[200]; memset(ib, 0, sizeof(ib));
    if (getParameterText(PX.inds, ib, sizeof(ib) - 1) == RTX_OK && ib[0] && ib[0] != ' ') S.inds = ib;   // blank (an older saved layout) = the default; "none" = off
}

void DealerProfile::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { D = dl::Data(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { D = dl::Data(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    long long st = dl::fileStamp(path);
    if (st >= 0 && st == loadedStamp && path == loadedPath) return;      // unchanged: keep what is parsed (no disk read)
    D = dl::Data();
    std::ifstream f(path.c_str()); if (!f.is_open()) { loadedPath.clear(); return; }
    std::stringstream ss; ss << f.rdbuf();
    D = dl::parseText(ss.str());
    loadedStamp = st; loadedPath = path;
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

    if (dl::anyUsd(D)) {   // ---- 2.0.0: net GEX per strike (MenthorQ's numbers)
        std::vector<dl::Node> vis;
        for (size_t i = 0; i < D.nodes.size(); i++) { short yy = yOf(D.nodes[i].k + off); if (yy >= pane.top && yy <= pane.bottom) vis.push_back(D.nodes[i]); }
        float umax = dl::usdMax(vis.empty() ? D.nodes : vis);
        int barH = hs - 2 * gap - 1; if (barH > 2 * nodeH + 2) barH = 2 * nodeH + 2; if (barH < 3) barH = 3;
        if (barH > S.font + 6) barH = S.font + 6;              // (2.0.2) a bar, never a block (NG / EU zoomed in: strikes 100-300 px apart)
        bool ctxOnly = (mkt == "CL" || mkt == "NG");             // (2.0.2) options data context only in these markets: dimmed
        short lastLabY = -10000;
        for (size_t i = 0; i < D.nodes.size(); i++) {
            const dl::Node& N = D.nodes[i];
            if (!N.hasUsd) continue;
            short y = yOf(N.k + off);
            if (y < pane.top - hs || y > pane.bottom + hs) continue;
            float ft = (S.fadefar && N.far) ? 0.55f : 0.0f;
            if (ctxOnly && ft < 0.35f) ft = 0.35f;
            int L = dl::barLen(N.usd, umax, W), L0 = dl::barLen(dl::odtePart(N), umax, W);
            short t = (short)(y - barH / 2), b = (short)(t + barH);
            COLOR c = fade(N.usd < 0 ? C_SELL : C_BUY, ft), c0 = fade(N.usd < 0 ? C_SELL : C_BUY, ft + (1.0f - ft) * 0.45f);
            if (L > 0) box((short)(anchor - L), t, anchor, b, c);
            if (L0 > 0) box((short)(anchor - L0), t, anchor, b, c0);          // today's 0DTE: the darker part next to the scale
            // (2.0.2) a value on every visible bar (NQ's 5-pt strikes made the bars too thin and every value was skipped); when bars
            // are closer than the text is tall, the biggest bar there keeps its value
            if (S.labels && L >= 4 && y >= pane.top && y <= pane.bottom) {
                std::string lab = dl::usdLabel(N.usd);
                int tw = textW(lab.c_str(), S.font, true);
                bool inside = barH >= S.font - 3 && L - L0 > 0 && dl::fits(tw, L - L0);
                bool bigger = false;                                     // a bigger bar within one text height keeps the value
                for (size_t q = 0; q < D.nodes.size() && !bigger; q++) {
                    if (q == i || !D.nodes[q].hasUsd) continue;
                    short yq = yOf(D.nodes[q].k + off);
                    if (std::abs((int)yq - (int)y) < S.font + 3 && std::fabs(D.nodes[q].usd) > std::fabs(N.usd)) bigger = true;
                }
                if (!bigger && std::abs((int)y - (int)lastLabY) >= S.font + 3) {
                    if (inside) textLJ((short)(anchor - L + 3), y, lab.c_str(), N.usd < 0 ? 0x00FFFFFF : C_DARK, S.font, true);
                    else textRJ((short)(anchor - L - 3), y, lab.c_str(), c, S.font, true);
                    lastLabY = y;
                }
            }
            for (size_t j = 0; j < D.tags.size(); j++) {   // KEY / TGT / MAG chips stay, left of the bar
                const dl::Data::Tag& T = D.tags[j];
                if (std::fabs(T.k - N.k) > step * 0.5f) continue;
                int fsz = S.font - 1; if (fsz < 7) fsz = 7;
                int tw2 = textW(T.text.c_str(), fsz, true) + 8;
                COLOR bg = T.col == 'A' ? 0x00C084FC : T.col == 'B' ? C_WALL : T.col == 'G' ? C_BUY : T.col == 'C' ? 0x0022D3EE : C_INK;
                short ch = (short)(fsz + 5);
                short x2 = (short)(anchor - L - 60);
                box((short)(x2 - tw2), (short)(y - ch / 2), x2, (short)(y + ch / 2), bg);
                textLJ((short)(x2 - tw2 + 4), y, T.text.c_str(), C_DARK, fsz, true);
            }
        }
        return;
    }
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
            if (dl::fits(textW(gl.c_str(), S.font, true), Lg)) textRJ((short)(anchor - 4), (short)((gt + gb) / 2), gl.c_str(), N.g < 0 ? 0x00FFFFFF : C_DARK, S.font, true);   // (1.3.9) white on purple
            if (dl::fits(textW(dlab.c_str(), S.font, true), Ld)) textRJ((short)(anchor - 4), (short)((dt_ + db) / 2), dlab.c_str(), N.d < 0 ? 0x00FFFFFF : C_DARK, S.font, true);
        }
        if (S.labels && !N.far && nodeH >= S.font - 3) {   // (1.2) the strength share right OUTSIDE each node: % of a normal 15 min of volume
            int reachG = Lg; if (S.snap && Ls > reachG) reachG = Ls;
            if (false && S.whisk) { int a2 = dl::barLen(N.lo, gmax, W), b2 = dl::barLen(N.hi, gmax, W); if (a2 > reachG) reachG = a2; if (b2 > reachG) reachG = b2; }
            // (1.2) [lightning if LIVE] [strength bars] [%]  right outside the node, coloured by band (Rassul 2026-09-29)
            if (Lg > 0 && N.gp >= 0) strengthMark((short)(anchor - reachG - 4), (short)((gt + gb) / 2), N.gp, N.live, S.font, gc);
            if (Ld > 0 && N.dp >= 0) strengthMark((short)(anchor - Ld - 4), (short)((dt_ + db) / 2), N.dp, false, S.font, dc);
        }
        // (1.4.0, Rassul 2026-09-30, mockup v26) the strikes the Analyst and the Read talk about: KEY (the level), TGT (the target
        // wall), MAG (the 0DTE magnet) - a small chip at the left end of the strike's gamma node
        for (size_t j = 0; j < D.tags.size(); j++) {
            const dl::Data::Tag& T = D.tags[j];
            if (std::fabs(T.k - N.k) > step * 0.5f) continue;
            int fsz = S.font - 1; if (fsz < 7) fsz = 7;
            int tw = textW(T.text.c_str(), fsz, true) + 8;
            COLOR bg = T.col == 'A' ? 0x00C084FC : T.col == 'B' ? C_WALL : T.col == 'G' ? C_BUY : T.col == 'C' ? 0x0022D3EE : C_INK;
            short ch = (short)(fsz + 5), cy = (short)((gt + gb) / 2);
            short x2 = Lg >= tw + 8 ? (short)(anchor - Lg + 3 + tw) : (short)(anchor - (Lg > 0 ? Lg : 0) - 60);
            box((short)(x2 - tw), (short)(cy - ch / 2), x2, (short)(cy + ch / 2), bg);
            textLJ((short)(x2 - tw + 4), cy, T.text.c_str(), C_DARK, fsz, true);
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
static COLOR bandCol(int b) { return b == 0 ? 0x006B7280 : b == 1 ? 0x00E5E7EB : b == 2 ? 0x00F97316 : 0x00FFFFFF; }   // (1.3.9) meaningful = ORANGE (yellow is the wall now)
static COLOR mix(COLOR c, float a)   // the colour at alpha a over the black chart (no line is ever black)
{
    int r = (int)(((c >> 16) & 0xFF) * a), g = (int)(((c >> 8) & 0xFF) * a), b = (int)((c & 0xFF) * a);
    return (COLOR)((r << 16) | (g << 8) | b);
}
static bool darkInk(COLOR c)        // dark text on a bright fill, white on a dark one (purple)
{
    float l = 0.299f * ((c >> 16) & 0xFF) + 0.587f * ((c >> 8) & 0xFF) + 0.114f * (c & 0xFF);
    return l >= 150.0f;
}
void DealerProfile::strengthMark(short rightX, short y, float pct, bool live, int font, COLOR base)
{
    // (1.5.0, option D - Rassul 2026-09-30, as the Reader's size chips) the % pill takes the colour of the node it sizes:
    // HEAVY 3%+ solid, BIG 1-3% half, MODERATE 0.25-1% faint fill + coloured text, LIGHT under 0.25% outline only
    int b = dl::strengthBars(pct);
    std::string t = dl::pctTxt(pct);
    int fsz = font - 1; if (fsz < 6) fsz = 6;
    int tw = textW(t.c_str(), fsz, true);
    short x = rightX;
    {
        COLOR fill = b == 3 ? base : b == 2 ? mix(base, 0.55f) : b == 1 ? mix(base, 0.22f) : 0x00000000;
        COLOR ink  = b == 3 ? (darkInk(base) ? 0x000B0F19 : 0x00FFFFFF) : b == 2 ? (darkInk(fill) ? 0x000B0F19 : 0x00FFFFFF) : base;
        COLOR edge = b == 0 ? mix(base, 0.6f) : fill;
        short padx = 6, h = (short)(fsz + 6);
        short w = (short)(tw + 2 * padx + 2);
        short l = (short)(rightX - w), t0 = (short)(y - h / 2), bt = (short)(t0 + h);
        setPen(edge, 1, P_SOLID);
        CBRUSH br(fill, PAT_SOLID); br.set();
        RCT rc; rc.set(l, t0, rightX, bt); rc.drawRounded(6, 6);
        {
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
    if (D.y > 2000) {                                            // (2.0.2) by date: the time-of-day difference wrapped (Friday 16:03 read as 21h on Saturday)
        struct tm a; memset(&a, 0, sizeof(a)); a.tm_year = D.y - 1900; a.tm_mon = D.mo - 1; a.tm_mday = D.d;
        a.tm_hour = (int)(D.asofSo / 3600); a.tm_min = (int)((long)(D.asofSo / 60) % 60); a.tm_isdst = -1;
        struct tm b = t; b.tm_isdst = -1;
        double d = difftime(mktime(&b), mktime(&a)) / 60.0 - cfg.clock;
        if (d > -600) age = d < 0 ? 0 : d;
    }
    if (age <= 10.0) return;
    char b[40]; if (age >= 2880) sprintf_s(b, sizeof(b), "DEALER DATA STALE %dd", (int)(age / 1440 + 0.5)); else if (age >= 90) sprintf_s(b, sizeof(b), "DEALER DATA STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "DEALER DATA STALE %dm", (int)(age + 0.5));
    RCT pane; pane.getPaneRect(false);
    int tw = textW(b, 10, true);
    short x = (short)(pane.right - tw - 90), y = (short)(pane.top + 6);
    RCT bg; bg.set(x, y, (short)(x + tw + 14), (short)(y + 18));
    bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x + 7), (short)(y + 9), b, 0x00FF9A8F, 10, true);
}

// (2.1.0, Rassul 2026-10-03: "for ES it should use SPX data ... SPX instead of ES") when the bars come from the index book
// (SPX for ES, QQQ for NQ, 08:30-15:00 CT) a small tag over the profile says so - the bars jump in size when the book changes
void DealerProfile::srcTag()
{
    if (D.srcBook.empty() || D.srcBook == mkt || D.nodes.empty()) return;
    RCT pane; pane.getPaneRect(false);
    std::string t = D.srcBook + " book";
    int tw = textW(t.c_str(), 9, true);
    short x = (short)(pane.right - tw - 12), y = (short)(pane.top + 30);
    RCT bg; bg.set(x, y, (short)(x + tw + 10), (short)(y + 16));
    bg.draw(1, 0x0022D3EE, 0x00062A33, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x + 5), (short)(y + 8), t.c_str(), 0x0022D3EE, 9, true);
}

void DealerProfile::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION," << DP_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nNODES," << D.nodes.size() << "\nOFFSET," << off << "\nBOOK," << D.srcBook << "\nRATIO," << D.srcRatio << "\nRATIO_MODE," << D.srcMode << "\nSTATE," << what
      << "\nREACH," << (cfg.width + 70) << "\n";   // (1.4.0) px the Profile takes from the scale's edge: the Read / Analyst stay left of it
}

// (1.3.7, Rassul 2026-09-30: the Read went blank overnight - MenthorQ's price freezes outside RTH) the chart's own last 200
// bars -> LRA-IRT-Bars-<MKT>.csv, so the Reader always has the live price, highs / lows and volume (sweeps, triggers) even at
// night. Written when a bar changes, at most every 15 s; raw chart prices plus the contract offset the Reader takes off.
void DealerProfile::exportBars()
{
    long n = getBarCount(); if (n < 2 || mkt.empty()) return;
    RTARRAY o(barOpen), h(barHigh), l(barLow), c(barClose);
    RTARRAYI v(barVolume);                                   // volume is an integer array in the SDK
    RTARRAYI dtm(barDateTime);
    float lc = c[(int)n - 1];
    time_t now = time(0);
    if (now - expT < 15) return;
    if (n == expN && lc == expC && now - expT < 60) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-IRT-Bars-" + mkt + ".csv";
    std::string tmp = path + ".tmp";
    // (1.5.0, Rassul 2026-09-30) his own indicators (cob_bull, cob_bear ... by name, set in the settings) ride along, one
    // column each, so the Reader can check its rebuilt triggers against the chart's - the chart is the source of truth
    std::vector<std::string> names; std::vector<RTARRAY*> arrs;
    {
        std::string cur; std::string all = cfg.inds + ",";
        for (size_t i = 0; i < all.size(); i++) {
            char ch = all[i];
            if (ch == ',' || ch == ';') {
                while (!cur.empty() && cur.back() == ' ') cur.pop_back();
                if (!cur.empty() && names.size() < 8) {
                    RTARRAY* a = new RTARRAY(fEmptyArray);
                    char nm[80]; memset(nm, 0, sizeof(nm)); strncpy_s(nm, sizeof(nm), cur.c_str(), _TRUNCATE);
                    if ((a->getChartIndicator(nm) == RTX_OK && a->count > 0) || (a->getCustomIndicator(nm, 1, true) == RTX_OK && a->count > 0)) { names.push_back(cur); arrs.push_back(a); } else delete a;
                }
                cur.clear();
            } else if (!(cur.empty() && ch == ' ') && ch != '|') cur += ch;
        }
    }
    std::string nl; for (size_t j = 0; j < names.size(); j++) nl += (j ? "," : "") + names[j];
    {
        std::ofstream f(tmp.c_str(), std::ios::trunc); if (!f.is_open()) { for (size_t j = 0; j < arrs.size(); j++) delete arrs[j]; return; }
        char b[200];
        sprintf_s(b, sizeof(b), "IRTBARS|2|%s|%s|%.6f|%ld|%s\n", mkt.c_str(), root.c_str(), (double)off, (long)now, nl.c_str()); f << b;
        int from = (int)n - 200; if (from < 0) from = 0;
        for (int i = from; i < (int)n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dtm[i], &t);
            sprintf_s(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d|%.6f|%.6f|%.6f|%.6f|%.0f\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                      t.tm_hour, t.tm_min, t.tm_sec, (double)o[i], (double)h[i], (double)l[i], (double)c[i], (double)v[i]);
            std::string row(b); if (!row.empty() && row.back() == '\n') row.pop_back();
            for (size_t j = 0; j < arrs.size(); j++) {
                long k = (long)i - ((long)n - arrs[j]->count);           // the indicator's array ends on the chart's last bar
                char vb[32]; if (k >= 0 && k < arrs[j]->count) sprintf_s(vb, sizeof(vb), "|%.4f", (double)(*arrs[j])[(int)k]); else sprintf_s(vb, sizeof(vb), "|");
                row += vb;
            }
            f << row << "\n";
        }
    }
    for (size_t j = 0; j < arrs.size(); j++) delete arrs[j];
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) == 0) { expN = n; expC = lc; expT = now; }
}

int DealerProfile::draw(void)
{
    load();
    alignContract();
    render(cfg);
    staleBadge();
    srcTag();
    exportBars();
    writeStatus(D.nodes.empty() ? "no data" : "drawn");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerProfile *p = new DealerProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Dealer Profile: what dealers must trade at each strike. The guide is below the settings.");
    p->setVersion("2.1.0");
    return p;
}
