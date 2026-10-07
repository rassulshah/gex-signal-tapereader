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
#include <map>
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

static const char* DP_VERSION = "2.4.0";   // 2.4.0 (2026-10-07): touch odds on each MenthorQ level (1h / by expiry / by close)   // 2.3.0 (2026-10-07): the MenthorQ key-level lines (FlexLevels replaced)   // 2.2.3 (20:29): width default 50 (all profiles fit on the right). 2.2.1 (2026-10-05 17:55): Profile width default 80 px. 2.2.0 (2026-10-05): per-chart settings + per-market bar export (one DLL object serves every chart)
//   // 2.1.0 (2026-10-03): SPX / QQQ book tag (file 2.0 SRC row)
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
struct Settings { int market = 0, width = 50, font = 9, clock = 0; bool labels = true, banner = false, legend = true, snap = true, whisk = true, fadefar = true;
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
    struct Exp { long n = 0; float c = 0; time_t t = 0; };
    std::map<std::string, Exp> expBy;                        // (2.2.0) the chart-bar export throttle, per market
    void exportBars();

    bool dialogReady();
    void syncSettings(bool fromDialog = false);
    // (2.2.7, Rassul 23:33-23:34 "KEY and MAGNET cross over into the delta profile column ... keep them in the dealer profile
    // column") the KEY / TGT / MAG chips stay, drawn INSIDE the 70 px the Profile reserves left of its bars (REACH), never past it.
    // KEY carries the strike's 1-hour GEX change (KEY +18%).
    void drawKeyPct(float k, float step, short anchor, short y, const Settings& S)
    {
        for (size_t j = 0; j < D.tags.size(); j++) {
            const dl::Data::Tag& T = D.tags[j];
            if (std::fabs(T.k - k) > step * 0.5f) continue;
            int fsz = S.font - 1; if (fsz < 7) fsz = 7;
            std::string txt = T.text;
            int tw = textW(txt.c_str(), fsz, true) + 8;
            while (tw > 62 && txt.size() > 3) { txt = txt.substr(0, txt.size() - 1); tw = textW(txt.c_str(), fsz, true) + 8; }   // never wider than the reserve
            COLOR bg = T.col == 'A' ? 0x00C084FC : T.col == 'B' ? C_WALL : T.col == 'G' ? C_BUY : T.col == 'C' ? 0x0022D3EE : C_INK;
            short ch = (short)(fsz + 5);
            short x2 = (short)(anchor - S.width - 36);                  // (2.2.8) clear of the bar values (e.g. 5.4M) left of a full bar
            box((short)(x2 - tw), (short)(y - ch / 2), x2, (short)(y + ch / 2), bg);
            textLJ((short)(x2 - tw + 4), y, txt.c_str(), C_DARK, fsz, true);
        }
    }
    std::string chartKey(); std::string savedKey; long long savedStamp = -2;
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
    // (2.3.0, Rassul 2026-10-06 21:02 "irt ... pauses every few minutes ... i think its because of flexlevels ... replace with
    // dealerprofile giving the levels"; 2026-10-07 the bridge log: FlexLevels' Remote File check hits the bridge every 61-86 s,
    // HEAD then GET, up to 11 s apart - his "slightly over a minute" pause) the MenthorQ key levels drawn HERE from the local
    // MenthorQLevels.csv the bridge already writes: no HTTP, re-read only when the file changes. Remove lsFlexLevels from the charts.
    struct MQL { float px = 0; std::string label; COLOR col = 0; int w = 1; };
    std::map<std::string, std::vector<MQL> > mql; long long mqlStamp = -2; time_t mqlChecked = 0;
    // (2.4.0) LRA-Touch-<MKT>.csv: TOUCH|code|price|P 1h|P by expiry|P by close|proven (lra.touch_prob, every 5 min)
    struct TP { float px = 0; std::string p1, pe, pc; bool proven = false; };
    std::map<std::string, std::vector<TP> > tps; std::map<std::string, long long> tpStamp; std::map<std::string, time_t> tpChecked, tpAt;
    void loadTouch(const std::string& m);
    void loadMQLevels();
    void drawMQLevels(const Settings& S);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerProfile::DealerProfile() : cppExtension() { off = 0.0f; lastBar = 0; }

bool DealerProfile::dialogReady() { int i = getListIndex(PX.market); return i >= 0 && i <= 7; }
// (2.2.4, Rassul 23:05 "the same thing is happening with the dealer profile") the GammaProfile way: read the saved values
// whenever they are populated (Width 30..900 is the test); a list that reads -1 keeps its last value
// (2.2.5, Rassul 23:06 labels vanish when the settings window closes) IRT fills list / check-box values only while the
// window is open. Read + SAVE them per chart then; use the saved copy while it is closed (survives restarts).
static std::string dpSettingsPath() { const char* up = getenv("USERPROFILE"); return std::string(up ? up : "C:") + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.settings.txt"; }
std::string DealerProfile::chartKey()
{
    // (fix 23:22) the settings window's copy of the plugin has NO bar size / chart label (its key read "EPZ26~~") while the
    // chart's copy has them, so the two never matched. Key = the market symbol only (charts of one market share settings).
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    std::string k = rs ? rs : "";
    for (size_t i = 0; i < k.size(); i++) if (k[i] == '|' || k[i] == '\n' || k[i] == '\r') k[i] = ' ';
    return k;
}
void DealerProfile::syncSettings(bool fromDialog)
{
    std::string key = chartKey();
    // the settings window's copy has no bar size: only there are the values real (on the chart every list reads 0 and
    // every box unchecked - which is why "Auto" passed the old test and blank values were saved)
    const char* pl = getPeriodicityLabel(); bool inDialog = !(pl && pl[0]);
    if (inDialog && fromDialog) {                       // saved ONLY from the settings window's callbacks
        readSettings(cfg);
        std::map<std::string, std::string> all; std::ifstream in(dpSettingsPath().c_str()); std::string ln;
        while (std::getline(in, ln)) { size_t p = ln.find('|'); if (p != std::string::npos) all[ln.substr(0, p)] = ln; }
        in.close();
        std::string inds = cfg.inds; for (size_t i = 0; i < inds.size(); i++) if (inds[i] == '|') inds[i] = ',';
        char b[200]; sprintf_s(b, sizeof(b), "|%d|%d|%d|%d|%d|%d|%d|%d|", cfg.market, cfg.width, cfg.font, cfg.clock, cfg.labels ? 1 : 0,
                               cfg.snap ? 1 : 0, cfg.fadefar ? 1 : 0, cfg.banner ? 1 : 0);
        std::string row = key + b + inds;
        if (all[key] != row) { all[key] = row; std::ofstream out(dpSettingsPath().c_str(), std::ios::trunc); for (auto& kv : all) out << kv.second << "\n"; }
        savedKey = key; return;
    }
    long long st = dl::fileStamp(dpSettingsPath());
    if (savedKey == key && st == savedStamp) return;
    savedStamp = st;
    std::ifstream in(dpSettingsPath().c_str()); std::string ln;
    while (std::getline(in, ln)) {
        std::vector<std::string> t = dl::split(ln, '|');
        if (t.size() >= 9 && t[0] == key) {
            cfg.market = atoi(t[1].c_str()); cfg.width = atoi(t[2].c_str()); cfg.font = atoi(t[3].c_str()); cfg.clock = atoi(t[4].c_str());
            cfg.labels = t[5] == "1"; cfg.snap = t[6] == "1"; cfg.fadefar = t[7] == "1"; cfg.banner = t[8] == "1";
            if (t.size() >= 10 && !t[9].empty()) cfg.inds = t[9];
            savedKey = key; return;
        }
    }
}
int DealerProfile::parmsLoad(void)  { syncSettings(true); return RTX_OK; }
int DealerProfile::parmsApply(void) { syncSettings(true); return RTX_OK; }
int DealerProfile::parmsUpdt(unsigned int) { syncSettings(true); return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.3.12, Rassul 2026-09-30) only real settings have a control; the guide is plain text (setLabelParameter) below.
    // IRT keeps saved values BY POSITION (the parameter version does not reset them): remove + re-add the indicator once.
    setParameterVersion(5);   // (2.2.2) BUMPED: resets saved charts to the 80 px width default
    setParameterDialogHeight(8);    // (2.0.1, Rassul 2026-10-02 19:12) the how-to text is gone - controls only
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.market  = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    // (2.0.3) NUMW (DealerLogic.h): the SDK default width (0) made the number boxes so narrow they showed "T"
    PX.width   = pc++; setIntegerParameter("Profile width px", 50, NUMW, SL);   // (2.2.1, Rassul 17:54 "too big") default 80
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
    { int v = getListIndex(PX.market); if (v >= 0 && v <= 7) S.market = v; }
    S.width = getIntegerValue(PX.width); if (S.width < 30) S.width = 50; if (S.width > 900) S.width = 900;   // (1.0.1) a first dialog showed 9: anything under 40 px is not a real width
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
            drawKeyPct(N.k, step, anchor, y, S);   // (2.2.7) no KEY / TGT / MAG chips - only the level strike's 1-hour % change
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
        // (2.2.7, Rassul 23:32 "why even have KEY" / 23:33 "KEY and MAGNET cross over into the delta profile column") the chips are
        // gone; the level strike shows only its 1-hour GEX change, inside the Profile's own reach
        drawKeyPct(N.k, step, anchor, (short)((gt + gb) / 2), S);
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
      << "\nREACH," << (cfg.width + 100) << "\n";   // (1.4.0) px the Profile takes from the scale's edge: the Read / Analyst stay left of it
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
    Exp& E = expBy[mkt];
    if (now - E.t < 15) return;
    if (n == E.n && lc == E.c && now - E.t < 60) return;
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
    if (std::rename(tmp.c_str(), path.c_str()) == 0) { E.n = n; E.c = lc; E.t = now; }
}

void DealerProfile::loadMQLevels()
{
    time_t now = time(nullptr);
    if (now - mqlChecked < 5 && mqlStamp != -2) return;      // a stat at most every 5 s
    mqlChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\MenthorQLevels.csv";
    long long st = dl::fileStamp(p);
    if (st == mqlStamp) return;
    std::ifstream f(p.c_str()); if (!f.is_open()) { mqlStamp = st; return; }
    std::map<std::string, std::vector<MQL> > m;
    std::string ln; bool head = true;
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (head) { head = false; if (ln.compare(0, 6, "SYMBOL") == 0) continue; }
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, ',')) c.push_back(x);
        if (c.size() < 5 || c[0].empty()) continue;
        MQL q; q.px = (float)atof(c[1].c_str()); q.label = c[2];
        long v = atol(c[3].c_str()); q.col = (COLOR)(v & 0x00FFFFFF); if (q.col == 0) q.col = 0x0022D3EE;   // never black
        q.w = atoi(c[4].c_str()); if (q.w < 1) q.w = 1; if (q.w > 3) q.w = 3;
        if (q.px > 0) m[c[0]].push_back(q);
    }
    mql.swap(m); mqlStamp = st;
}

void DealerProfile::loadTouch(const std::string& m)
{
    time_t now = time(nullptr);
    if (now - tpChecked[m] < 5 && tpStamp.count(m)) return;
    tpChecked[m] = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Touch-" + m + ".csv";
    long long st = dl::fileStamp(p);
    if (tpStamp.count(m) && st == tpStamp[m]) return;
    std::vector<TP> v;
    std::ifstream f(p.c_str());
    std::string ln;
    while (f.is_open() && std::getline(f, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, '|')) c.push_back(x);
        if (c.size() >= 7 && c[0] == "TOUCH") { TP t; t.px = (float)atof(c[2].c_str()); t.p1 = c[3]; t.pe = c[4]; t.pc = c[5]; t.proven = c[6] == "1"; v.push_back(t); }
    }
    tps[m].swap(v); tpStamp[m] = st; tpAt[m] = st > 0 ? (time_t)(st / 1000003LL) : 0;   // the file's modified time
}

void DealerProfile::drawMQLevels(const Settings& S)
{
    loadMQLevels();
    if (mql.empty() || mkt.empty()) return;
    const std::vector<MQL>* L = nullptr;
    auto it = mql.find(root);
    if (it != mql.end()) L = &it->second;
    else for (auto& kv : mql) if (dl::marketForRoot(kv.first) == mkt) { L = &kv.second; break; }   // a rolled contract: same market
    if (!L) return;
    RCT pane; pane.getPaneRect(false);
    long nb = getBarCount(); if (nb < 1) return;
    int dec = mkt == "GC" ? 1 : mkt == "HG" ? 4 : mkt == "EU" ? 5 : mkt == "NG" ? 3 : 2;
    int fs = S.font; if (fs < 7) fs = 7;
    short xMid = (short)(pane.left + (pane.right - pane.left) / 2);
    for (size_t i = 0; i < L->size(); i++) {
        const MQL& q = (*L)[i];
        PNT pp; pp.set((int)(nb - 1), q.px); short y = pp.v;
        if (y <= pane.top || y >= pane.bottom) continue;
        line(pane.left, y, pane.right, y, q.col, q.w);
        char b[160]; snprintf(b, sizeof(b), "%.*f %s", dec, q.px, q.label.c_str());
        // (2.4.0, Rassul 2026-10-07 12:57 "start showing it now on the levels") the chance price touches this level: in the next
        // hour, by today's 0DTE expiry (when it comes before the close) and by the close; "?" until the market's numbers are proven
        {
            loadTouch(mkt);
            bool fresh = tpAt[mkt] > 0 && time(nullptr) - tpAt[mkt] < 20 * 60;    // stale odds (the bridge stopped) are not shown
            const std::vector<TP>& T = tps[mkt];
            float tol = q.px * 2e-5f;
            for (size_t k = 0; fresh && k < T.size(); k++) {
                if (std::fabs(T[k].px + off - q.px) > tol && std::fabs(T[k].px - q.px) > tol) continue;
                const char* qm = T[k].proven ? "" : "?";
                std::string add = "  1h " + T[k].p1 + "%" + qm;
                if (!T[k].pe.empty() && T[k].pe != T[k].pc) add += "  exp " + T[k].pe + "%" + qm;
                if (!T[k].pc.empty()) add += "  close " + T[k].pc + "%" + qm;
                strncat_s(b, sizeof(b), add.c_str(), _TRUNCATE);
                break;
            }
        }
        int tw = textW(b, fs - 1, false) + 10; short h = (short)(fs + 6);
        short l = (short)(xMid - tw / 2), r = (short)(xMid + tw / 2), t = (short)(y - h / 2), bt = (short)(y + h / 2);
        box(l, t, r, bt, C_DARK);
        line(l, t, r, t, q.col, 1); line(r, t, r, bt, q.col, 1); line(r, bt, l, bt, q.col, 1); line(l, bt, l, t, q.col, 1);
        textLJ((short)(l + 5), y, b, q.col, fs - 1, false);
    }
}

int DealerProfile::draw(void)
{
    // (2.2.0, 2026-10-05) IRT runs ONE object of this DLL for every chart, so the settings read when another chart's dialog
    // was last applied leaked here: the HG chart drew / exported as GC (LRA-IRT-Bars-GC.csv held copper bars, root CPEZ26) and
    // the profile flipped between markets ("disappears and reappears"). Read THIS chart's settings on every draw.
    syncSettings();
    load();
    alignContract();
    drawMQLevels(cfg);                        // (2.3.0) under the profile: the key-level lines first
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
    p->setVersion("2.4.0");   // (2.4.0) touch odds on the MenthorQ levels;   // (2.3.0) draws the MenthorQ key levels (replaces lsFlexLevels and its 1-minute HTTP check)
    return p;
}
