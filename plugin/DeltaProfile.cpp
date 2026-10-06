/********************************************************************************
 *  DeltaProfile.cpp  --  Investor/RT RTX extension  lsDeltaProfile  (v1.0.0, 2026-10-05)
 *
 *  THE DELTA PROFILE (Rassul 2026-10-05: "determine if accumulation or distribution is happening as we come close to a key
 *  level ... show me absorption ... qualify and quantify"; "abbreviated text so it is not taking up too much space"; "make the
 *  size 80 wide and editable"). Layout v2: candles -> DLT (this) -> LIQ -> GEX (Dealer Profile) on the right of the chart.
 *
 *  One bar per price row over the last 30 min: bought - sold, GREEN to the right = buyers won the row, RED to the left =
 *  sellers; a faint bar behind = the volume traded there. One code at the level the Reader works on, with its strength:
 *     Ab absorption · Ex exhaustion · Tr trapped · Acc accumulation · Dst distribution · In initiative (against the trade)
 *     plain = watch, "+" = likely, a check mark = confirmed. Green / red = it supports a long / short; amber = Ex.
 *  The numbers and words are in the Dealer Read's FLOW lines and the settings description - nothing else on the chart.
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Delta-<MKT>.csv, written with every Reader build by the LRA
 *  (analytics/lra/delta_profile.py) from the FootprintReader's 1-min footprint. Lines (| separated):
 *     ASOF|yyyy-mm-dd HH:MM:SS|from|to   LEVEL|price|support/resistance|name   BADGE|code|strength(''/+/v)   ROW|price|delta|volume
 *  Settings: Market, Width px (default 80), Gap from Dealer Profile px (negative = closer), Font size, Values on the biggest bars.
 *  1.1.0 (18:00): Range (Last 30 min / Session / Day from 08:30), Ticks per row, Sides One / Both - as Rassul's examples.
 *  1.0.3 (17:52): red and green bars both grow toward the price chart from one base line. Place Right (by the Dealer Profile) or Left (the chart's left edge); "Gap px" from that edge.
 *  1.0.1 (17:15): the gap may be negative; the bars show whenever the footprint has data (the badge needs a level).
 *  Position: right of the candles, just left of the Dealer Profile (its status file says how wide it is: REACH).
 *  IRT runs ONE object per DLL for every chart: this chart's settings are read on every draw. Never black lines.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

static const char* DLT_VERSION = "1.3.5";
static const COLOR C_BUY   = 0x0022C55E;
static const COLOR C_SELL  = 0x00EF4444;
static const COLOR C_AMBER = 0x00F59E0B;
static const COLOR C_VOL   = 0x00243040;   // faint volume bar over the black chart
static const COLOR C_AXIS  = 0x00334155;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;

struct DIdx { int market, width, gap, font, labels, place, range, group, sides, offlvl, face; };
static DIdx DX;
struct DSet { int market = 0, width = 50, gap = 8, font = 9, place = 0, range = 0, group = 1, sides = 0; int face = 0; bool labels = true, offlvl = true; };

struct DRow { float px = 0, d = 0, v = 0; };
struct DData {
    bool ok = false; std::string asof, from, to, side, name, code, strength; float level = 0;
    std::vector<DRow> rows, srows, drows, hrows; float tick = 0; int levelN = 0; float levelX = 0;
    struct DMark { float px = 0; std::string code, side; int n = 0; float x = 0; }; std::vector<DMark> marks;
};

class DeltaProfile : public cppExtension {
public:
    DeltaProfile() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int);
    virtual int draw(void);

    DSet cfg; DData D; std::string mkt, root; int lastBar = 0;
    long long loadedStamp = -2; std::string loadedPath;
    bool dialogReady();
    std::string rootKey();
    void syncSettings(bool fromDialog = false);
    std::string chartKey(); std::string savedKey; long long savedStamp = -2;
    void readSettings(DSet& S);
    void load();
    int dealerReach();
    void render(const DSet& S);
    void writeStatus(const char* what);
    short yOf(float price) { PNT p; p.set(lastBar, price); return p.v; }
    void box(short l, short t, short r, short b, COLOR c) { if (r <= l || b <= t) return; RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void line(short x1, short y1, short x2, short y2, COLOR c, int w)
    {
        setPen(c, (short)w, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
        PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
    }
    int textW(const char* s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s, -1); }
    void textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + 400), (short)(yy + sz)); rc.drawText(s, false, false);
    }
    void textRJ(short rx, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
        RCT rc; rc.set((short)(rx - 400), (short)(yy - sz), rx, (short)(yy + sz)); rc.drawText(s, false, true);
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

bool DeltaProfile::dialogReady() { int i = getListIndex(DX.market); return i >= 0 && i <= 7; }
static std::map<std::string, DSet>& perChart() { static std::map<std::string, DSet> m; return m; }   // (1.2.1) settings per chart
// (1.3.2, Rassul 22:16 "it keeps changing and losing its labels") two charts of the SAME market (the ES 3-min and 1-min)
// shared one settings slot keyed by the root symbol, so each chart drew with the other's Range / labels. Key = this
// indicator instance (each chart's copy has its own object) + its market.
std::string DeltaProfile::rootKey() { char rb[32] = {0}; const char* rs = getRootSymbol(rb); char p[32]; sprintf_s(p, sizeof(p), "%p|", (void*)this); return std::string(p) + (rs ? rs : ""); }
// (1.3.3, Rassul 23:04 "as soon as i open the indicator it changes ... when i close the settings it reverts back") the
// GammaProfile way, proven since v0.78: read THIS chart's saved values on every draw whenever they are populated (Width
// 30..600 is the test), keep the last good ones otherwise. No shared or keyed store - each chart reads its own values.
// (1.3.4, Rassul 23:06 "clicking the indicator shows labels ... when i close the setting the labels go away") IRT only
// fills the list / check-box values while the settings window is open (outside it lists read -1, boxes read unchecked).
// So: while the window is open (the Market list reads 0..7) read everything and SAVE it to a small file per chart
// (market + bar size + chart); while it is closed, use the saved copy. Each chart keeps its own settings across restarts.
static std::string settingsPath() { const char* up = getenv("USERPROFILE"); return std::string(up ? up : "C:") + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.settings.txt"; }
std::string DeltaProfile::chartKey()
{
    // (fix 23:22) the settings window's copy of the plugin has NO bar size / chart label (its key read "EPZ26~~") while the
    // chart's copy has them, so the two never matched. Key = the market symbol only (charts of one market share settings).
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    std::string k = rs ? rs : "";
    for (size_t i = 0; i < k.size(); i++) if (k[i] == '|' || k[i] == '\n' || k[i] == '\r') k[i] = ' ';
    return k;
}
void DeltaProfile::syncSettings(bool fromDialog)
{
    std::string key = chartKey();
    // the settings window's copy has no bar size: only there are the values real (on the chart every list reads 0 and
    // every box unchecked - which is why "Auto" passed the old test and blank values were saved)
    const char* pl = getPeriodicityLabel(); bool inDialog = !(pl && pl[0]);
    if (inDialog && fromDialog) {                       // saved ONLY from the settings window's callbacks                                   // the window is open: the values are real - read and save
        readSettings(cfg);
        std::map<std::string, std::string> all; std::ifstream in(settingsPath().c_str()); std::string ln;
        while (std::getline(in, ln)) { size_t p = ln.find('|'); if (p != std::string::npos) all[ln.substr(0, p)] = ln; }
        in.close();
        char b[200]; sprintf_s(b, sizeof(b), "|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d", cfg.market, cfg.width, cfg.gap, cfg.font, cfg.labels ? 1 : 0,
                               cfg.place, cfg.range, cfg.group, cfg.sides, cfg.offlvl ? 1 : 0, cfg.face);
        std::string row = key + b;
        if (all[key] != row) { all[key] = row; std::ofstream out(settingsPath().c_str(), std::ios::trunc); for (auto& kv : all) out << kv.second << "\n"; }
        savedKey = key; return;
    }
    long long st = dl::fileStamp(settingsPath());
    if (savedKey == key && st == savedStamp) return;
    savedStamp = st;                           // already using this chart's saved copy
    std::ifstream in(settingsPath().c_str()); std::string ln;
    while (std::getline(in, ln)) {
        std::vector<std::string> t = dl::split(ln, '|');
        if (t.size() >= 12 && t[0] == key) {
            cfg.market = atoi(t[1].c_str()); cfg.width = atoi(t[2].c_str()); cfg.gap = atoi(t[3].c_str()); cfg.font = atoi(t[4].c_str());
            cfg.labels = t[5] == "1"; cfg.place = atoi(t[6].c_str()); cfg.range = atoi(t[7].c_str()); cfg.group = atoi(t[8].c_str());
            cfg.sides = atoi(t[9].c_str()); cfg.offlvl = t[10] == "1"; cfg.face = atoi(t[11].c_str());
            savedKey = key; return;
        }
    }
}
int DeltaProfile::parmsLoad(void)  { syncSettings(true); return RTX_OK; }
int DeltaProfile::parmsApply(void) { syncSettings(true); return RTX_OK; }
int DeltaProfile::parmsUpdt(unsigned int) { syncSettings(true); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(6);   // (1.1.3) BUMPED: 1.1.x added Place / Range / Ticks per row / Sides - charts saved with the 1.0 settings crashed IRT when the dialog opened (msglog 19:05); resets to the defaults
    setParameterDialogHeight(8);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    DX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    DX.width  = pc++; setIntegerParameter("Width px", 50, NUMW, SL);   // (1.1.6, Rassul 20:29) 50 so DLT / LIQ / GEX all fit on the right
    DX.gap    = pc++; setIntegerParameter("Gap px", 8, NUMW);
    DX.font   = pc++; setIntegerParameter("Font size (pt)", 9, NUMW, SL);
    DX.labels = pc++; setBoolParameter("Values on the biggest bars", true);
    DX.place  = pc++; setListParameter("Place", 0, "Right;Left");   // (1.2.1) Right first = the default (all profiles on the right)
    DX.range  = pc++; setListParameter("Range", 0, "Last 30 min;Last 60 min;Session;Day from 08:30");   // (1.1.0) as the examples Rassul sent
    DX.group  = pc++; setIntegerParameter("Ticks per row (0 = auto)", 1, NUMW);
    DX.sides  = pc++; setListParameter("Sides", 0, "One;Both");
    DX.offlvl = pc++; setBoolParameter("Letters at pivots and big ones", true);
    DX.face   = pc++; setListParameter("Face", 0, "Price;Dealer Profile");   // (1.3.0, Rassul 21:55) default: toward price   // (1.0.3) Left = at the chart's left edge, apart from the Dealer Profile
    return RTX_OK;
}

void DeltaProfile::readSettings(DSet& S)
{
    { int v = getListIndex(DX.market); if (v >= 0 && v <= 7) S.market = v; }   // (1.3.3) an unreadable list (-1) keeps the last value
    S.width = getIntegerValue(DX.width); if (S.width < 30) S.width = 50; if (S.width > 600) S.width = 600;
    S.gap = getIntegerValue(DX.gap); if (S.gap < -300) S.gap = -300; if (S.gap > 400) S.gap = 400;   // (1.0.1) negative = closer to / over the Dealer Profile
    S.font = getIntegerValue(DX.font); if (S.font < 6) S.font = 9; if (S.font > 18) S.font = 18;
    S.labels = isBoxChecked(DX.labels) != 0;
    { int pi = getListIndex(DX.place); if (pi == 0 || pi == 1) S.place = pi; }   // list Right;Left -> 0 = Right, 1 = Left
    { int v = getListIndex(DX.range); if (v >= 0 && v <= 3) S.range = v; }
    S.group = getIntegerValue(DX.group); if (S.group < 0) S.group = 0; if (S.group > 500) S.group = 500;
    { int v = getListIndex(DX.sides); if (v == 0 || v == 1) S.sides = v; }
    S.offlvl = isBoxChecked(DX.offlvl) != 0;
    { int v = getListIndex(DX.face); if (v == 0 || v == 1) S.face = v; }
}

void DeltaProfile::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { D = DData(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { D = DData(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Delta-" + mkt + ".csv";
    long long st = dl::fileStamp(path);
    if (st >= 0 && st == loadedStamp && path == loadedPath) return;      // re-read only when the file changed
    D = DData();
    std::ifstream f(path.c_str()); if (!f.is_open()) { loadedPath.clear(); return; }
    std::string ln;
    while (std::getline(f, ln)) {
        std::vector<std::string> t = dl::split(ln, '|');
        if (t.empty()) continue;
        if (t[0] == "ASOF" && t.size() >= 4) { D.asof = t[1]; D.from = t[2]; D.to = t[3]; }
        else if (t[0] == "LEVEL" && t.size() >= 3) { D.level = (float)atof(t[1].c_str()); D.side = t[2]; if (t.size() >= 4) D.name = t[3]; }
        else if (t[0] == "BADGE" && t.size() >= 2) { D.code = t[1]; D.strength = t.size() >= 3 ? t[2] : ""; }
        else if (t[0] == "TICK" && t.size() >= 2) D.tick = (float)atof(t[1].c_str());
        else if (t[0] == "LEVELN" && t.size() >= 2) D.levelN = atoi(t[1].c_str());
        else if (t[0] == "LEVELX" && t.size() >= 2) D.levelX = (float)atof(t[1].c_str());
        else if (t[0] == "MARK" && t.size() >= 5) { DData::DMark k; k.px = (float)atof(t[1].c_str()); k.code = t[2]; k.side = t[3]; k.n = atoi(t[4].c_str()); if (t.size() >= 6) k.x = (float)atof(t[5].c_str()); D.marks.push_back(k); }
        else if ((t[0] == "ROW" || t[0] == "SROW" || t[0] == "RROW" || t[0] == "HROW") && t.size() >= 4) {
            DRow r; r.px = (float)atof(t[1].c_str()); r.d = (float)atof(t[2].c_str()); r.v = (float)atof(t[3].c_str());
            (t[0] == "ROW" ? D.rows : t[0] == "SROW" ? D.srows : t[0] == "HROW" ? D.hrows : D.drows).push_back(r);
        }
    }
    D.ok = !D.rows.empty();
    loadedStamp = st; loadedPath = path;
}

// how far the Dealer Profile reaches in from the price scale (its status file); 0 when it is not on this chart's market
int DeltaProfile::dealerReach()
{
    const char* up = getenv("USERPROFILE"); if (!up) return 0;
    std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt").c_str());
    if (!f.is_open()) return 0;
    std::string ln, m; int reach = 0;
    while (std::getline(f, ln)) {
        std::vector<std::string> t = dl::split(ln, ',');
        if (t.size() >= 2 && t[0] == "MARKET") m = t[1];
        if (t.size() >= 2 && t[0] == "REACH") reach = atoi(t[1].c_str());
    }
    // (1.1.5, Rassul 20:16 "it seems to shift around") the status file is shared by every chart, so it often holds ANOTHER
    // market's reach (0 here) and the column jumped. Keep the last good reach per market instead.
    static std::map<std::string, int> last;
    if (m == mkt && reach > 0 && reach < 1200) last[mkt] = reach;
    return last.count(mkt) ? last[mkt] : 150;
}

void DeltaProfile::render(const DSet& S)
{
    long n = getBarCount(); if (n < 2) return;
    lastBar = (int)n - 1;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    // (1.2.0, Rassul 21:17 mockup v2) DLT faces RIGHT (toward the Liquidity Profile); the letter column (the seam) is on
    // its right: [DLT ->][letters] then LIQ and the Dealer Profile
    const short SEAM = 62;   // (1.3.1) room for "A 120 2x"
    short right, left;
    if (S.place == 1) {                                             // Left: from the pane's left edge + gap
        left = (short)(pane.left + 4 + (S.gap > 0 ? S.gap : 0)); right = (short)(left + S.width);
        if (right + SEAM >= paneR - 40) return;
    } else {                                                        // Right: the block ends just left of the Dealer Profile
        right = (short)(paneR - 2 - dealerReach() - S.gap - SEAM); left = (short)(right - S.width);
        if (left <= pane.left + 40) return;
    }
    // (1.3.0) Face Price (default): [letters][<- DLT] bars grow LEFT toward the candles, letters on the candle side.
    //         Face Dealer Profile: [DLT ->][letters] bars grow RIGHT toward the Dealer Profile, letters after them.
    bool toPrice = S.face == 0;
    bool growLeft = (S.place == 0) ? toPrice : !toPrice;          // price is LEFT of a Right column, RIGHT of a Left column
    if (growLeft) { left = (short)(left + SEAM); right = (short)(right + SEAM); }     // same block, letters first
    int dir = growLeft ? -1 : 1;
    short base = growLeft ? right : left;
    short sL = growLeft ? (short)(left - SEAM) : right, sR = (short)(sL + SEAM);      // the letter column (at the bars' tips side)
    textLJ((short)(left + 2), (short)(pane.top + 8), "DLT", C_MUTED, 8, true);
    if (S.sides != 1) line(base, (short)(pane.top + 16), base, pane.bottom, C_AXIS, 1);
    if (growLeft) line(sL, (short)(pane.top + 16), sL, pane.bottom, C_AXIS, 1); else line(sR, (short)(pane.top + 16), sR, pane.bottom, C_AXIS, 1);
    short mid = (short)(left + S.width / 2);
    if (!D.ok) return;

    // (1.1.0) Range: the last 30 min, the whole session, or the day from 08:30
    const std::vector<DRow>& R = S.range == 1 && !D.hrows.empty() ? D.hrows : S.range == 2 && !D.srows.empty() ? D.srows : S.range == 3 && !D.drows.empty() ? D.drows : D.rows;   // (1.1.4) + Last 60 min
    // rows -> buckets: N ticks per row, or (0 = auto) one bar per few pixels
    int bh = S.font - 3; if (bh < 3) bh = 3;
    float g = (S.group > 0 && D.tick > 0) ? D.tick * S.group : 0;
    struct BK { DRow r; short t = 0, b = 0; };
    std::map<int, BK> B;
    for (size_t i = 0; i < R.size(); i++) {
        int k; short yt, yb;
        if (g > 0) {
            k = (int)std::floor(R[i].px / g + 1e-6);
            yt = yOf((k + 1) * g); yb = (short)(yOf(k * g) - 1); if (yb <= yt) yb = (short)(yt + 1);
        } else {
            short y = yOf(R[i].px);
            k = (y - pane.top) / bh; yt = (short)(pane.top + k * bh); yb = (short)(yt + bh - 1);
        }
        if (yb < pane.top + 16 || yt > pane.bottom) continue;
        BK& x = B[k]; x.r.d += R[i].d; x.r.v += R[i].v; x.r.px = R[i].px; x.t = yt; x.b = yb;
    }
    float dmax = 1, vmax = 1;
    for (auto& kv : B) { if (std::fabs(kv.second.r.d) > dmax) dmax = std::fabs(kv.second.r.d); if (kv.second.r.v > vmax) vmax = kv.second.r.v; }
    bool both = S.sides == 1;                                       // Both: buyers right / sellers left of a centre line
    int full = both ? S.width / 2 - 1 : S.width - 2;
    if (both) line(mid, (short)(pane.top + 16), mid, pane.bottom, C_AXIS, 1);
    std::vector<std::pair<float, int>> big;
    for (auto& kv : B) big.push_back(std::make_pair(std::fabs(kv.second.r.d), kv.first));
    std::sort(big.begin(), big.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });
    std::map<int, bool> lab; for (size_t i = 0; i < big.size() && i < 3; i++) lab[big[i].second] = true;
    for (auto& kv : B) {
        const DRow& r = kv.second.r; short t = kv.second.t, b = kv.second.b;
        int L = (int)(std::fabs(r.d) / dmax * full); if (L < 1 && r.d != 0) L = 1;
        int lv = (int)(r.v / vmax * (both ? S.width : full));
        short de;
        if (both) {
            box((short)(mid - lv / 2), t, (short)(mid + lv / 2), b, C_VOL);
            de = r.d > 0 ? (short)(mid + L) : (short)(mid - L);
            if (r.d > 0) box(mid, t, de, b, C_BUY); else if (r.d < 0) box(de, t, mid, b, C_SELL);
        } else {
            short ve = (short)(base + dir * lv);
            box(dir > 0 ? base : ve, t, dir > 0 ? ve : base, b, C_VOL);
            de = (short)(base + dir * L);
            if (r.d != 0) box(dir > 0 ? base : de, t, dir > 0 ? de : base, b, r.d > 0 ? C_BUY : C_SELL);
        }
        if (S.labels && lab[kv.first] && r.d != 0) {
            char s[24]; float a = std::fabs(r.d);
            if (a >= 1000) sprintf_s(s, sizeof(s), "%s%.1fk", r.d > 0 ? "+" : "-", a / 1000); else sprintf_s(s, sizeof(s), "%s%.0f", r.d > 0 ? "+" : "-", a);
            short yc = (short)((t + b) / 2);
            bool rightSide = both ? r.d > 0 : dir > 0;
            if (rightSide) textLJ((short)(de + 2), yc, s, C_INK, S.font - 1, false);
            else textRJ((short)(de - 2), yc, s, C_INK, S.font - 1, false);
        }
    }
    // (1.2.0) letters in the seam column, on the row they describe: the key level full colour (+ contracts: A 250),
    // pivots / big ones dimmer (A 180). '?' = setting up.
    // (1.3.1, Rassul 22:01 "A 120 (2x)") volume vs the same minute's normal: " 2x" (one decimal under 10x)
    auto xs = [](float x) -> std::string { if (x < 0) return ""; char b[16];
        if (x < 0.1f) sprintf_s(b, sizeof(b), " <0.1x"); else if (x < 10) sprintf_s(b, sizeof(b), " %.1fx", x); else sprintf_s(b, sizeof(b), " %.0fx", x);
        std::string s = b; if (s.size() > 4 && s.substr(s.size() - 3) == ".0x") s = s.substr(0, s.size() - 3) + "x"; return s; };
    auto drawLetter = [&](float px, std::string t, COLOR col) {
        short y = yOf(px); if (y < pane.top + 16 || y > pane.bottom) return;
        textLJ((short)(sL + 3), y, t.c_str(), col, S.font, true);
    };
    if (S.offlvl) for (size_t i = 0; i < D.marks.size(); i++) {
        const DData::DMark& k = D.marks[i];
        bool sup = k.side == "support";
        COLOR col = k.code == "E" ? 0x00A16207 : (sup ? 0x00166534 : 0x00991B1B);     // dim amber / green / red
        std::string t = k.code; if (k.n > 0) { char b[16]; sprintf_s(b, sizeof(b), " %d", k.n); t += b; }
        if (k.x > 0 || k.code == "E") t += xs(k.x);
        drawLetter(k.px, t, col);
    }
    if (!D.code.empty() && D.level > 0) {
        bool up = D.side == "support";
        COLOR col = D.code == "Ex" ? C_AMBER : D.code == "In" ? (up ? C_SELL : C_BUY) : (up ? C_BUY : C_SELL);
        std::string t = D.code == "Ab" ? "A" : D.code == "Ex" ? "E" : D.code == "Tr" ? "T" : D.code == "In" ? "I" : D.code.substr(0, 1);
        if (D.strength == "?") t += "?";
        if (D.code == "Ab" && D.levelN > 0) { char b[16]; sprintf_s(b, sizeof(b), " %d", D.levelN); t += b; }
        if (D.levelX > 0) t += xs(D.levelX);
        drawLetter(D.level, t, col);
    }
}

void DeltaProfile::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.status.txt").c_str(), std::ios::trunc);
    if (!f.is_open()) return;
    f << "VERSION," << DLT_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nROWS," << D.rows.size() << "\nASOF," << D.asof
      << "\nBADGE," << D.code << D.strength << "\nWIDTH," << cfg.width << "\nSTATE," << what << "\n";
}

int DeltaProfile::draw(void)
{
    // (1.1.5, Rassul 20:17 "even the amount of data that is shown changes") one DLL object serves every chart: settings
    // read from one chart's dialog were used by the next chart that drew. Keep them per chart (root symbol), and reload
    // the data file whenever the market changes.
    syncSettings();
    load();
    render(cfg);
    writeStatus(D.ok ? "drawn" : "no data");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DeltaProfile *p = new DeltaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Delta Profile (DLT)");
    p->setVersion("1.3.5");
    return p;
}
