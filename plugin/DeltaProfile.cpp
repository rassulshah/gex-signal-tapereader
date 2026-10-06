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
#include <ctime>

static const char* DLT_VERSION = "1.8.0";
static const COLOR C_BUY   = 0x0022C55E;
static const COLOR C_SELL  = 0x00EF4444;
static const COLOR C_AMBER = 0x00F59E0B;
static const COLOR C_VOL   = 0x00243040;   // faint volume bar over the black chart
static const COLOR C_AXIS  = 0x00334155;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;

struct DIdx { int market, width, gap, font, labels, place, range, group, sides, offlvl, face, mins; };
static DIdx DX;
struct DSet { int market = 0, width = 50, gap = 8, font = 9, place = 0, range = 0, group = 1, sides = 0; int face = 0; int mins = 60; bool labels = true, offlvl = true; };
// (1.7.0, Rassul 12:00 "think about the settings and combinations so it all makes sense") range 0 = Last N minutes (N = mins),
// 1 = Session, 2 = Day from 08:30 - Minutes is used ONLY with Last N minutes; face 0 = bars grow Left, 1 = Right

struct DRow { float px = 0, d = 0, v = 0; };
struct DData {
    bool ok = false; std::string asof, from, to, side, name, code, strength; float level = 0;
    std::vector<DRow> rows, srows, drows, hrows; std::map<int, std::vector<DRow>> mrows;   // (1.6.0) MROW|minutes|px|d|v
    float tick = 0; int levelN = 0; float levelX = 0;
    struct DMark { float px = 0; std::string code, side; int n = 0; float x = 0; }; std::vector<DMark> marks;
    struct DNode { std::string rk, tpk; float px = 0, d = 0, x = 0; std::string code, side; }; std::vector<DNode> nodes;
    std::string built; float recAge = -1;              // (1.5.3) when the Reader wrote the file + the recorder's heartbeat age then   // (1.5.0) states of the big delta nodes
    struct DZone { float lo = 0, hi = 0; std::string code, side; int share = 0; }; std::vector<DZone> zones;   // (1.4.0) Dst? / Dst / Acc / T zones
};

class DeltaProfile : public cppExtension {
public:
    DeltaProfile() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int);
    virtual int draw(void);

    DSet cfg; DData D; std::string mkt, root; int lastBar = 0;
    long long loadedStamp = -2; std::string loadedPath; std::string lastWant;   // (1.6.0)
    std::string drawnRange; int drawnRows = 0;   // (1.7.0) for the status file
    void wantMinutes();
    int barForMinute(const std::string& tpk);   // (1.8.0) the chart bar holding a minute (stamped at its END, "YYYY-MM-DD HH:MM")
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
        char b[200]; sprintf_s(b, sizeof(b), "|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|2", cfg.market, cfg.width, cfg.gap, cfg.font, cfg.labels ? 1 : 0,
                               cfg.place, cfg.range, cfg.group, cfg.sides, cfg.offlvl ? 1 : 0, cfg.face, cfg.mins);
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
            cfg.mins = t.size() >= 13 ? atoi(t[12].c_str()) : 60; if (cfg.mins < 5 || cfg.mins > 1440) cfg.mins = 60;
            if (!(t.size() >= 14 && t[13] == "2")) {          // (1.7.0) an older row: Range 30 / 60 / Session / Day / Minutes, Face Price / Dealer
                int r = cfg.range;
                if (r == 0) { cfg.range = 0; cfg.mins = 30; } else if (r == 1) { cfg.range = 0; cfg.mins = 60; }
                else if (r == 2) cfg.range = 1; else if (r == 3) cfg.range = 2; else cfg.range = 0;
                bool toPrice = cfg.face == 0; bool gl = cfg.place == 0 ? toPrice : !toPrice;
                cfg.face = gl ? 0 : 1;
            }
            savedKey = key; return;
        }
    }
}
int DeltaProfile::parmsLoad(void)  { syncSettings(true); return RTX_OK; }
int DeltaProfile::parmsApply(void) { syncSettings(true); return RTX_OK; }
int DeltaProfile::parmsUpdt(unsigned int) { syncSettings(true); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(9);   // (1.7.0) the settings window re-laid out: Range = Last N minutes / Session / Day + Minutes, Face Left / Right
    // was 8 (1.6.0 Minutes), 7 (1.4.1 letters on), 6 (1.1.3: new parameters without a bump crashed IRT when the dialog opened)
    setParameterDialogHeight(9);
    const short SL = kParmAppendSameLine;
    const short LW = 120;     // list width - every list the same, so the rows line up
    int pc = 0;
    DX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU", LW);
    DX.range  = pc++; setListParameter("Range", 0, "Last N minutes;Session;Day from 08:30", LW);       // default Last N minutes
    DX.mins   = pc++; setIntegerParameter("N minutes", 60, NUMW, SL);                                   // used only with Last N minutes
    DX.place  = pc++; setListParameter("Place", 0, "Right;Left", LW);                                    // Right first = the default
    DX.face   = pc++; setListParameter("Face", 0, "Left;Right", NUMW + 20, SL);                          // bars grow Left (toward price) by default
    DX.group  = pc++; setIntegerParameter("Ticks per row", 1, NUMW);
    DX.sides  = pc++; setListParameter("Sides", 0, "One;Both", NUMW + 20, SL);
    DX.width  = pc++; setIntegerParameter("Width px", 50, NUMW);
    DX.gap    = pc++; setIntegerParameter("Gap px", 8, NUMW, SL);
    DX.font   = pc++; setIntegerParameter("Font pt", 9, NUMW, SL);
    DX.labels = pc++; setBoolParameter("Amounts on the 3 biggest bars", true);
    DX.offlvl = pc++; setBoolParameter("Letters (A?  A  I  E, zones)", true);
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
    { int v = getListIndex(DX.range); if (v >= 0 && v <= 2) S.range = v; }
    S.mins = getIntegerValue(DX.mins); if (S.mins < 5) S.mins = 5; if (S.mins > 1440) S.mins = 1440;
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
        else if (t[0] == "ZONE" && t.size() >= 6) { DData::DZone z; z.lo = (float)atof(t[1].c_str()); z.hi = (float)atof(t[2].c_str()); z.code = t[3]; z.side = t[4]; z.share = atoi(t[5].c_str()); D.zones.push_back(z); }
        else if (t[0] == "LEVELX" && t.size() >= 2) D.levelX = (float)atof(t[1].c_str());
        else if (t[0] == "BUILT" && t.size() >= 3) { D.built = t[1]; D.recAge = (float)atof(t[2].c_str()); }
        else if (t[0] == "NODE" && t.size() >= 7) { DData::DNode n; n.rk = t[1]; n.px = (float)atof(t[2].c_str()); n.d = (float)atof(t[3].c_str()); n.code = t[4]; n.side = t[5]; n.x = (float)atof(t[6].c_str()); if (t.size() >= 8) n.tpk = t[7]; D.nodes.push_back(n); }
        else if (t[0] == "MROW" && t.size() >= 5) { DRow r; r.px = (float)atof(t[2].c_str()); r.d = (float)atof(t[3].c_str()); r.v = (float)atof(t[4].c_str()); D.mrows[atoi(t[1].c_str())].push_back(r); }
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
    const short SEAM = 38;   // (1.3.1) room for "A 120 2x"; (1.5.5, Rassul 10:05 "taking up too much space") 62 -> 38: letters now sit
                             // right beside their amount instead of in a fixed column
    const short VALS = 28;   // (1.5.5) 34 -> 28   // (1.3.6, Rassul 23:37 "i dont want it to overlap the volume amount") room for the bar values between the letters and the bars
    short right, left;
    if (S.place == 1) {                                             // Left: from the pane's left edge + gap
        left = (short)(pane.left + 4 + (S.gap > 0 ? S.gap : 0)); right = (short)(left + S.width);
        if (right + SEAM >= paneR - 40) return;
    } else {                                                        // Right: the block ends just left of the Dealer Profile
        right = (short)(paneR - 2 - dealerReach() - S.gap - SEAM - VALS); left = (short)(right - S.width);
        if (left <= pane.left + 40) return;
    }
    // (1.3.0) Face Price (default): [letters][<- DLT] bars grow LEFT toward the candles, letters on the candle side.
    //         Face Dealer Profile: [DLT ->][letters] bars grow RIGHT toward the Dealer Profile, letters after them.
    bool growLeft = S.face == 0;                                   // (1.7.0) Face = the direction the bars grow: Left / Right
    if (growLeft) { left = (short)(left + SEAM + VALS); right = (short)(right + SEAM + VALS); }     // letters, values, then bars
    int dir = growLeft ? -1 : 1;
    short base = growLeft ? right : left;
    short sL = growLeft ? (short)(left - SEAM - VALS) : (short)(right + VALS), sR = (short)(sL + SEAM);      // the letter column, clear of the bar values
    textLJ((short)(left + 2), (short)(pane.top + 8), "DLT", C_MUTED, 8, true);
    // (1.5.1, Rassul 09:34 "is there a mechanism to show whether there is missed data") the file's last footprint minute vs
    // the chart clock: > 10 min old = the recorder or the Reader stopped -> a red STALE chip under the header
    // (1.5.3) stale = the Reader stopped (BUILT is old) or the recorder stopped (its heartbeat was old when the file was
    // built) - NOT a quiet market with no trades for 10 minutes. Files without BUILT fall back to the last footprint minute.
    {
        const std::string& st = D.built.size() >= 16 ? D.built : D.asof;
        struct tm a; memset(&a, 0, sizeof(a));
        if (st.size() >= 16 && sscanf_s(st.c_str(), "%d-%d-%d %d:%d", &a.tm_year, &a.tm_mon, &a.tm_mday, &a.tm_hour, &a.tm_min) == 5) {
            a.tm_year -= 1900; a.tm_mon -= 1; a.tm_isdst = -1;
            RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t); t.tm_isdst = -1;
            double age = difftime(mktime(&t), mktime(&a)) / 60.0;
            if (age < 0) age = 0;
            if (D.built.size() >= 16 && D.recAge > 0) age += D.recAge;
            // (1.6.1, HG 10:29 -> 11:13: the recorder ran but its tab was not drawn, so no new bars) the last footprint minute
            // more than 20 min behind the build is a data gap too (20, not 10: a thin market can go 10 min without a trade)
            if (D.built.size() >= 16 && D.asof.size() >= 16) {
                struct tm q; memset(&q, 0, sizeof(q));
                if (sscanf_s(D.asof.c_str(), "%d-%d-%d %d:%d", &q.tm_year, &q.tm_mon, &q.tm_mday, &q.tm_hour, &q.tm_min) == 5) {
                    q.tm_year -= 1900; q.tm_mon -= 1; q.tm_isdst = -1;
                    double gap = difftime(mktime(&t), mktime(&q)) / 60.0;
                    if (gap > 20.0 && gap > age) age = gap;
                }
            }
            if (age > 10.0 && age < 60.0 * 24 * 30) {
                char b[24]; if (age >= 2880) sprintf_s(b, sizeof(b), "STALE %dd", (int)(age / 1440 + 0.5)); else if (age >= 90) sprintf_s(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
                int tw = textW(b, 8, true);
                short x = (short)(left + 1), y = (short)(pane.top + 18);
                RCT bg; bg.set(x, y, (short)(x + tw + 6), (short)(y + 13));
                bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
                textLJ((short)(x + 3), (short)(y + 6), b, 0x00FF9A8F, 8, true);
            }
        }
    }
    if (S.sides != 1) line(base, (short)(pane.top + 16), base, pane.bottom, C_AXIS, 1);
    if (growLeft) line(sL, (short)(pane.top + 16), sL, pane.bottom, C_AXIS, 1); else line(sR, (short)(pane.top + 16), sR, pane.bottom, C_AXIS, 1);
    short mid = (short)(left + S.width / 2);
    if (!D.ok) return;

    // (1.1.0) Range: the last 30 min, the whole session, or the day from 08:30
    // (1.7.0) Last N minutes: 30 / 60 come with every build; any other N is built once a chart asks for it (want file) -
    // until then the nearest window that exists is drawn and the status file says so
    auto mit = D.mrows.find(S.mins);
    bool haveN = mit != D.mrows.end() && !mit->second.empty();
    const std::vector<DRow>& R =
        S.range == 1 ? (D.srows.empty() ? D.rows : D.srows) :
        S.range == 2 ? (D.drows.empty() ? D.rows : D.drows) :
        S.mins == 30 ? D.rows : S.mins == 60 ? (D.hrows.empty() ? D.rows : D.hrows) :
        haveN ? mit->second : (S.mins > 45 && !D.hrows.empty() ? D.hrows : D.rows);
    drawnRange = S.range == 1 ? "Session" : S.range == 2 ? "Day" : (S.mins == 30 || S.mins == 60 || haveN) ? "Last " + std::to_string(S.mins) + " min" : "Last " + std::to_string(S.mins) + " min (building - showing " + (S.mins > 45 ? "60" : "30") + ")";
    drawnRows = (int)R.size();
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
    struct LB { short t, b; float d; short edge; }; std::vector<LB> labd;
    std::map<int, short> edgeOf;                                   // (1.5.5) per row: the outer edge of the bar tip / its amount     // (1.5.0) the labelled nodes - their letters go on the same rows
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
        short edge = both ? (growLeft ? (short)(mid - full - 1) : (short)(mid + full + 1)) : de;
        if (S.labels && lab[kv.first] && r.d != 0) {
            char s[24]; float a = std::fabs(r.d);
            if (a >= 1000) sprintf_s(s, sizeof(s), "%s%.1fk", r.d > 0 ? "+" : "-", a / 1000); else sprintf_s(s, sizeof(s), "%s%.0f", r.d > 0 ? "+" : "-", a);
            short yc = (short)((t + b) / 2);
            bool rightSide = both ? r.d > 0 : dir > 0;
            int tw = textW(s, S.font - 1, false);
            if (rightSide) textLJ((short)(de + 2), yc, s, C_INK, S.font - 1, false);
            else textRJ((short)(de - 2), yc, s, C_INK, S.font - 1, false);
            if (!both) edge = rightSide ? (short)(de + 2 + tw) : (short)(de - 2 - tw);
        }
        edgeOf[kv.first] = edge;
        if (lab[kv.first] && r.d != 0) { LB lb; lb.t = t; lb.b = b; lb.d = r.d; lb.edge = edge; labd.push_back(lb); }
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
    // (1.4.0, Rassul 08:18 "build") zones: a bracket over the zone's rows in the letter column + ONE label - Dst? 70% while
    // forming, Dst 70% once price proved it, T if it failed (they are trapped). Always shown (the zone is the read).
    // (1.5.5) zones: a bracket just outside the zone's bars / amounts and ONE label beside it (no fixed column)
    for (size_t i = 0; i < D.zones.size(); i++) {
        const DData::DZone& z = D.zones[i];
        short yt = yOf(z.hi + D.tick * 0.5f), yb = yOf(z.lo - D.tick * 0.5f);
        if (yb < pane.top + 16 || yt > pane.bottom) continue;
        short ex = growLeft ? base : base;
        bool any = false;
        for (auto& kv : B) {
            if (kv.second.b < yt || kv.second.t > yb) continue;
            short e = edgeOf[kv.first];
            if (!any) { ex = e; any = true; } else ex = growLeft ? (e < ex ? e : ex) : (e > ex ? e : ex);
        }
        COLOR col = z.side == "support" ? 0x0086EFAC : 0x00FCA5A5;
        std::string t = z.code; if (z.code != "T" && z.code != "A" && z.share > 0) { char b[16]; sprintf_s(b, sizeof(b), " %d%%", z.share); t += b; }
        short bx = growLeft ? (short)(ex - 4) : (short)(ex + 4);
        short tk = growLeft ? (short)(bx + 3) : (short)(bx - 3);
        line(bx, yt, bx, yb, col, 1); line(bx, yt, tk, yt, col, 1); line(bx, yb, tk, yb, col, 1);
        if (growLeft) textRJ((short)(bx - 3), (short)((yt + yb) / 2), t.c_str(), col, S.font, true);
        else textLJ((short)(bx + 3), (short)((yt + yb) / 2), t.c_str(), col, S.font, true);
    }
    // (1.5.0, Rassul 09:16-09:18 "we are looking at the high delta nodes ... the ones that stick out and have the amounts on
    // them") ONE letter per labelled node, on its row: A? (price has not left it), A (the absorbers held it), T (they are
    // trapped). Colour = the side that won. The state comes from the Reader (NODE lines) for the Range this chart shows.
    if (S.offlvl) {
        std::string rk = S.range == 1 ? "S" : S.range == 2 ? "D" : S.mins == 30 ? "R" : S.mins == 60 ? "H" :
                         haveN ? "M" + std::to_string(S.mins) : (S.mins > 45 ? "H" : "R");
        std::sort(labd.begin(), labd.end(), [](const LB& p, const LB& q) { return p.t < q.t; });
        short lastY = -1000;
        for (size_t i = 0; i < labd.size(); i++) {
            const DData::DNode* best = nullptr;
            for (size_t j = 0; j < D.nodes.size(); j++) {
                const DData::DNode& n = D.nodes[j];
                if (n.rk != rk || (n.d < 0) != (labd[i].d < 0)) continue;
                short yn = yOf(n.px); if (yn < labd[i].t - 1 || yn > labd[i].b + 1) continue;
                if (!best || std::fabs(n.d) > std::fabs(best->d)) best = &n;
            }
            if (!best) continue;
            bool inZone = false;                                  // (1.5.3) a node inside a Dst / Acc zone: the zone's label is the read
            for (size_t z = 0; z < D.zones.size(); z++) if (best->px >= D.zones[z].lo - D.tick * 0.5f && best->px <= D.zones[z].hi + D.tick * 0.5f) inZone = true;
            if (inZone) continue;
            COLOR col = best->side == "support" ? 0x0086EFAC : 0x00FCA5A5;
            std::string t = best->code; if (best->code != "T" && best->x > 0) t += xs(best->x);
            short y = (short)((labd[i].t + labd[i].b) / 2);
            if (y - lastY < S.font + 2) y = (short)(lastY + S.font + 2);   // two labelled rows touching: stack, never overlap
            lastY = y;
            if (growLeft) textRJ((short)(labd[i].edge - 4), y, t.c_str(), col, S.font, true);    // (1.5.5) right beside its amount
            else textLJ((short)(labd[i].edge + 4), y, t.c_str(), col, S.font, true);
            // (1.8.0, Rassul 12:11 "draws a mark to identify which candle had the absorption ... in choppy price action") a ring on
            // the candle that holds the node's hardest-hitting minute, at the node's price, in the letter's colour
            int bb = barForMinute(best->tpk);
            if (bb >= 0) {
                PNT pp; pp.set(bb, best->px, kBarCenter);
                if (pp.h > pane.left && pp.h < pane.right && pp.v > pane.top + 16 && pp.v < pane.bottom) {
                    setPen(col, 2, P_SOLID);
                    CBRUSH hb(col, PAT_HOLLOW); hb.set();
                    RCT rr; rr.set((short)(pp.h - 6), (short)(pp.v - 6), (short)(pp.h + 6), (short)(pp.v + 6));
                    rr.drawOval(DRAW_OPAQUE);
                }
            }
        }
    }
    if (!D.code.empty() && D.level > 0) {
        bool up = D.side == "support";
        COLOR col = D.code == "Ex" ? C_AMBER : D.code == "In" ? (up ? C_SELL : C_BUY) : (up ? C_BUY : C_SELL);
        std::string t = D.code == "Ab" ? "A" : D.code == "Ex" ? "E" : D.code == "Tr" ? "A" : D.code == "In" ? "I" : D.code.substr(0, 1);
        if (D.strength == "?") t += "?";
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
      << "\nRANGE," << drawnRange << "\nDRAWN_ROWS," << drawnRows << "\nBADGE," << D.code << D.strength << "\nWIDTH," << cfg.width << "\nSTATE," << what << "\n";
}

// (1.6.0) Range = Minutes: tell the Reader which windows this market's charts want (lsFlexLevels\DeltaProfile.want-<MKT>.txt,
// newest 4); it writes MROW / NODE M<minutes> lines for each on its next build (within 5 min)
void DeltaProfile::wantMinutes()
{
    if (cfg.range != 0 || cfg.mins == 30 || cfg.mins == 60 || mkt.empty()) return;
    std::string tag = mkt + ":" + std::to_string(cfg.mins);
    if (tag == lastWant) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.want-" + mkt + ".txt";
    std::vector<int> v; { std::ifstream f(p.c_str()); int x; while (f >> x) if (x != cfg.mins && x >= 5 && x <= 1440) v.push_back(x); }
    v.push_back(cfg.mins); while (v.size() > 4) v.erase(v.begin());
    std::ofstream f(p.c_str(), std::ios::trunc); for (size_t i = 0; i < v.size(); i++) f << v[i] << "\n";
    lastWant = tag;
}

int DeltaProfile::barForMinute(const std::string& tpk)
{
    int y, mo, d, h, mi;
    if (tpk.size() < 16 || sscanf_s(tpk.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5) return -1;
    double s = h * 3600.0 + mi * 60.0 - 60.0;                  // the minute STARTS one minute before its stamp
    if (s < 0) return -1;
    long n = getBarCount(); if (n < 1) return -1;
    RTARRAYI dt(barDateTime);
    int from = (int)n - 3000; if (from < 0) from = 0;
    for (int i = (int)n - 1; i >= from; i--) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        int ty = t.tm_year + 1900, tm_ = t.tm_mon + 1;
        if (ty < y || (ty == y && (tm_ < mo || (tm_ == mo && t.tm_mday < d)))) break;    // past the day
        if (ty != y || tm_ != mo || t.tm_mday != d) continue;
        double s0 = t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec;
        if (s0 <= s) return i;                                 // bars are stamped at their start: the first one at or before the minute
    }
    return -1;
}

int DeltaProfile::draw(void)
{
    // (1.1.5, Rassul 20:17 "even the amount of data that is shown changes") one DLL object serves every chart: settings
    // read from one chart's dialog were used by the next chart that drew. Keep them per chart (root symbol), and reload
    // the data file whenever the market changes.
    syncSettings();
    load();
    wantMinutes();
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
    p->setVersion(DLT_VERSION);   // (1.7.0) the settings window showed 1.4.0
    return p;
}
