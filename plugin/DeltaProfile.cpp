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
#include <chrono>

static const char* DLT_VERSION = "2.2.0";   // (2.2.0) a ring only where the profile shows its letter now (native only);   // (2.1.0) Acc / Dst zones and the tick size native too;   // (2.0.2) the key level's A / I needs a big row too;   // (2.0.1) a letter only on a BIG row: 2x+ the average row (absorption needs size);   // (2.0.0) the A / A? / I / I? letters, rings and ratios are computed IN the plugin from the bars it draws (no file);   // (1.9.5) every ratio = the drawn row's delta vs the profile's average row, so a bigger bar always shows a bigger x;   // (1.9.4) the 3 biggest rows always carry their amount; a hidden I node its grey ratio;   // (1.9.3) one side, facing left, placed right; (1.9.2)   // (1.9.2) the 3 letter slots skip hidden initiative rows
static const COLOR C_BUY   = 0x0022C55E;
static const COLOR C_SELL  = 0x00EF4444;
static const COLOR C_AMBER = 0x00F59E0B;
static const COLOR C_VOL   = 0x00243040;   // faint volume bar over the black chart
static const COLOR C_AXIS  = 0x00334155;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;

struct DIdx { int market, width, gap, font, labels, place, range, group, sides, offlvl, face, mins, showI; };
static DIdx DX;
struct DSet { int market = 0, width = 50, gap = 8, font = 9, place = 0, range = 0, group = 1, sides = 0; int face = 0; int mins = 60; bool labels = true, offlvl = true, showI = false; };   // (1.8.3) showI: initiative letters off by default
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
    struct DZone { float lo = 0, hi = 0; std::string code, side; int share = 0; }; std::vector<DZone> zones;
    struct DRing { float px = 0, x = 0; std::string code, side, tpk; }; std::vector<DRing> rings;   // (1.9.1) the session's candle rings   // (1.4.0) Dst? / Dst / Acc / T zones
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
    std::vector<DRow> liveRows; long liveKey = -1; int liveRange = -1, liveMins = -1;   // (1.9.0) the live profile + its cache key
    std::string liveRoot; long long liveAt = 0; bool liveOff = false;   // (1.9.1) per chart, at most once a second, off after a fault
    std::vector<long long> barStamp; long barStampKey = -1;            // (1.9.1) bar close stamps for the candle rings
    // (2.0.0) the bars of the live window, oldest first: chart index, close time, OHLC and the delta at each price (tick key)
    struct LBar { int i = 0; time_t t = 0; float o = 0, h = 0, l = 0, c = 0; std::vector<std::pair<long long, float>> d; };
    std::vector<LBar> liveBars; float liveTick = 0;
    struct NState { std::string code, side; int peak = -1; bool ok = false; };
    NState classifyNative(float lo, float hi, float d, bool showI);
    std::string stampOf(int bar);
    void trace(const char* what);
    static int readBarVap(RTARRAYP* VP, int i, VOLPROFILE* out, int cap);
    bool buildLive(const DSet& S);
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
        std::map<std::string, std::string> all; std::ifstream in(settingsPath().c_str()); std::string ln;
        while (std::getline(in, ln)) { size_t p = ln.find('|'); if (p != std::string::npos) all[ln.substr(0, p)] = ln; }
        in.close();
        // (1.9.3, Rassul 22:33 "you made the positive and negative go in different directions" / 22:38 "the profile should be
        // facing left and placed on the right by default") a row saved before 1.9.3 is moved ONCE to Sides One, Face Left, Place
        // Right - in the window too, so pressing OK does not bring Both back. Later choices are kept (marker 3).
        static bool migrating = false;
        if (!migrating && all.count(key)) {
            std::vector<std::string> t0 = dl::split(all[key], '|');
            if (!(t0.size() >= 14 && t0[13] == "3")) { migrating = true; setListIndex(DX.sides, 0); setListIndex(DX.face, 0); setListIndex(DX.place, 0); migrating = false; }
        }
        readSettings(cfg);
        char b[200]; sprintf_s(b, sizeof(b), "|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|3|%d", cfg.market, cfg.width, cfg.gap, cfg.font, cfg.labels ? 1 : 0,
                               cfg.place, cfg.range, cfg.group, cfg.sides, cfg.offlvl ? 1 : 0, cfg.face, cfg.mins, cfg.showI ? 1 : 0);
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
            cfg.showI = t.size() >= 15 && t[14] == "1";
            if (!(t.size() >= 14 && t[13] == "3")) {          // (1.9.3) before 1.9.3: one side, facing left (toward price), placed right
                cfg.sides = 0; cfg.face = 0; cfg.place = 0;
            }
            if (!(t.size() >= 14 && (t[13] == "2" || t[13] == "3"))) {   // (1.7.0) an older row: Range 30 / 60 / Session / Day / Minutes, Face Price / Dealer
                int r = cfg.range;
                if (r == 0) { cfg.range = 0; cfg.mins = 30; } else if (r == 1) { cfg.range = 0; cfg.mins = 60; }
                else if (r == 2) cfg.range = 1; else if (r == 3) cfg.range = 2; else cfg.range = 0;
                bool toPrice = cfg.face == 0; bool gl = cfg.place == 0 ? toPrice : !toPrice;
                cfg.face = gl ? 0 : 1;
                cfg.sides = 0; cfg.face = 0; cfg.place = 0;      // (1.9.3) and then the new defaults
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
    setParameterVersion(10);   // (1.8.3) + "Initiative letters (I)" - off by default
    // was: setParameterVersion(9);   // (1.7.0) the settings window re-laid out: Range = Last N minutes / Session / Day + Minutes, Face Left / Right
    // was 8 (1.6.0 Minutes), 7 (1.4.1 letters on), 6 (1.1.3: new parameters without a bump crashed IRT when the dialog opened)
    setParameterDialogHeight(10);
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
    DX.offlvl = pc++; setBoolParameter("Absorption letters (A?  A, zones)", true);
    DX.showI  = pc++; setBoolParameter("Initiative letters (I?  I)", false);   // (1.8.3, Rassul 14:28 "only show absorption by default")
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
    S.showI = isBoxChecked(DX.showI) != 0;
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
        else if (t[0] == "RING" && t.size() >= 6) { DData::DRing g; g.px = (float)atof(t[1].c_str()); g.code = t[2]; g.side = t[3]; g.x = (float)atof(t[4].c_str()); g.tpk = t[5]; D.rings.push_back(g); }
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
    // (1.9.0, Rassul 14:52-14:55 "ok build it") the BARS are built LIVE from the chart's own volume at price (IRT's footprint,
    // every tick); the file only supplies the letters / zones / rings. Falls back to the file's rows when VAP is not available.
    bool live = buildLive(S);
    if (!D.ok && !live) return;

    // (1.1.0) Range: the last 30 min, the whole session, or the day from 08:30
    // (1.7.0) Last N minutes: 30 / 60 come with every build; any other N is built once a chart asks for it (want file) -
    // until then the nearest window that exists is drawn and the status file says so
    auto mit = D.mrows.find(S.mins);
    bool haveN = mit != D.mrows.end() && !mit->second.empty();
    const std::vector<DRow>& R = live ? liveRows :
        S.range == 1 ? (D.srows.empty() ? D.rows : D.srows) :
        S.range == 2 ? (D.drows.empty() ? D.rows : D.drows) :
        S.mins == 30 ? D.rows : S.mins == 60 ? (D.hrows.empty() ? D.rows : D.hrows) :
        haveN ? mit->second : (S.mins > 45 && !D.hrows.empty() ? D.hrows : D.rows);
    drawnRange = S.range == 1 ? "Session" : S.range == 2 ? "Day" : (S.mins == 30 || S.mins == 60 || haveN) ? "Last " + std::to_string(S.mins) + " min" : "Last " + std::to_string(S.mins) + " min (building - showing " + (S.mins > 45 ? "60" : "30") + ")";
    if (live) drawnRange += " LIVE";
    drawnRows = (int)R.size();
    // rows -> buckets: N ticks per row, or (0 = auto) one bar per few pixels
    int bh = S.font - 3; if (bh < 3) bh = 3;
    float g = (S.group > 0 && D.tick > 0) ? D.tick * S.group : 0;
    struct BK { DRow r; short t = 0, b = 0; float lo = 1e30f, hi = -1e30f; };
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
        BK& x = B[k]; x.r.d += R[i].d; x.r.v += R[i].v; x.r.px = R[i].px; x.t = yt; x.b = yb; if (R[i].px < x.lo) x.lo = R[i].px; if (R[i].px > x.hi) x.hi = R[i].px;
    }
    float dmax = 1, vmax = 1;
    for (auto& kv : B) { if (std::fabs(kv.second.r.d) > dmax) dmax = std::fabs(kv.second.r.d); if (kv.second.r.v > vmax) vmax = kv.second.r.v; }
    bool both = S.sides == 1;                                       // Both: buyers right / sellers left of a centre line
    int full = both ? S.width / 2 - 1 : S.width - 2;
    if (both) line(mid, (short)(pane.top + 16), mid, pane.bottom, C_AXIS, 1);
    std::vector<std::pair<float, int>> big;
    for (auto& kv : B) big.push_back(std::make_pair(std::fabs(kv.second.r.d), kv.first));
    // (1.9.5, Rassul 2026-10-07 11:07 "it is showing 7.4 for a node that is smaller than a larger node that has 7.1 ... you still
    // havent fixed this") the x beside a letter came from the file's node (volume vs that minute's normal, over the node's own
    // window), not from the bar drawn here - so a smaller bar could show a bigger x. Now every x on the profile is the DRAWN row's
    // |delta| / the average |delta| of the drawn rows: a bigger bar always carries a bigger number.
    float avgD = 0; int nD = 0;
    for (auto& kv : B) if (kv.second.r.d != 0) { avgD += std::fabs(kv.second.r.d); nD++; }
    avgD = nD ? avgD / nD : 0;
    auto rowX = [&](float d) -> float { return avgD > 0 ? std::fabs(d) / avgD : -1.0f; };
    const float MIN_X = 2.0f;                                      // (2.0.1) the smallest row that can carry a letter
    std::sort(big.begin(), big.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });
    // (1.9.2, Rassul 22:08 "why doesnt the delta profile show the A and the circles are there") the 3 labelled rows were the 3
    // biggest |delta| rows of ANY kind - with initiative letters off, an I row used a slot and showed nothing (HG 22:05: I 6.643,
    // I? 6.630 took 2 of the 3, the confirmed A at 6.6285 got no letter). Now a row whose node is hidden (I / I? with the
    // initiative letters off) does not take a slot: the 3 biggest SHOWN nodes get their letters.
    const std::string rk0 = S.range == 1 ? "S" : S.range == 2 ? "D" : S.mins == 30 ? "R" : S.mins == 60 ? "H" :
                            haveN ? "M" + std::to_string(S.mins) : (S.mins > 45 ? "H" : "R");
    // (2.0.0) live bars: the nodes are classified HERE from the drawn buckets (the 10 biggest), not read from the file, so every
    // letter / ring / ratio describes the bar it sits on. The file's NODE / RING lines are used only when live bars are off.
    bool nat = live && !liveBars.empty();
    std::vector<DData::DNode> natNodes;
    if (nat) {
        for (size_t i = 0; i < big.size() && i < 10; i++) {
            const BK& e = B[big[i].second]; if (e.r.d == 0) continue;
            if (rowX(e.r.d) < MIN_X) break;                       // (2.0.1) sorted biggest first: the rest are smaller still
            NState ns = classifyNative(e.lo, e.hi, e.r.d, S.showI);
            if (!ns.ok) continue;
            DData::DNode n; n.rk = rk0; n.px = (e.lo + e.hi) * 0.5f; n.d = e.r.d; n.code = ns.code; n.side = ns.side;
            n.x = rowX(e.r.d); n.tpk = ns.peak >= 0 ? stampOf(ns.peak) : "";
            natNodes.push_back(n);
        }
        // the session's rings, remembered per chart (root) so a node that leaves the window keeps its ring (1.9.1 rule)
        static std::map<std::string, std::vector<DData::DRing>> mem; static std::map<std::string, std::string> memDay;
        std::string day = stampOf(lastBar).substr(0, 10);
        {   // a session runs 17:00 -> 16:00: bars from 17:00 on belong to the next day's session
            RTARRAYI dt_(barDateTime); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt_[lastBar], &t);
            if (t.tm_hour >= 17) { t.tm_mday += 1; t.tm_isdst = -1; mktime(&t); char b[16]; sprintf_s(b, sizeof(b), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday); day = b; }
        }
        std::vector<DData::DRing>& M = mem[root];
        if (memDay[root] != day) { M.clear(); memDay[root] = day; }
        int kept = 0;
        for (size_t i = 0; i < natNodes.size() && kept < 3; i++) {   // (2.0.1) only the (up to) 3 letters shown become rings
            const DData::DNode& n = natNodes[i]; if (n.tpk.empty()) continue;
            if (!S.showI && !n.code.empty() && n.code[0] == 'I') continue;
            kept++;
            bool found = false;
            for (size_t j = 0; j < M.size(); j++) if (M[j].tpk == n.tpk && std::fabs(M[j].px - n.px) <= (D.tick > 0 ? D.tick : 0.0001f) * 2) { M[j].code = n.code; M[j].side = n.side; M[j].px = n.px; found = true; break; }
            if (!found) { DData::DRing g; g.px = n.px; g.code = n.code; g.side = n.side; g.tpk = n.tpk; g.x = n.x; M.push_back(g); }
        }
        if (M.size() > 80) M.erase(M.begin(), M.begin() + (M.size() - 80));
        D.rings = M;                                           // drawn by the ring pass below
        // (2.1.0, Rassul 16:39 "make sure that the delta profile and signals are native so they dont rely on external files")
        // the Acc / Dst zones, built HERE from the last 60 min of this chart's bars (lra/delta_profile.find_zones): touching rows
        // of heavy one-sided delta (each >= 25% of the biggest), red in the top quarter of the range (Dst) or green in the
        // bottom quarter (Acc), holding >= 40% of that side's net. Their state uses the same move / hold / candle-close rule
        // as the nodes: held the zone's way = Dst / Acc, absorbed = A, not yet = Dst? / Acc?
        float tkz = liveTick;                                  // the bars' price keys are in ticks only when the tick is known
        if (tkz > 0) {
            time_t tEnd = liveBars.back().t; std::map<long long, float> pd;
            for (size_t b_ = 0; b_ < liveBars.size(); b_++) if (liveBars[b_].t > tEnd - 3600)
                for (size_t k = 0; k < liveBars[b_].d.size(); k++) pd[liveBars[b_].d[k].first] += liveBars[b_].d[k].second;
            std::vector<std::pair<float, float>> P;                // price, delta - low to high
            for (auto& kv : pd) P.push_back(std::make_pair((float)(kv.first * tkz), kv.second));
            std::vector<DData::DZone> Z;
            if (P.size() >= 5) {
                float bigD = 0, totS = 0, totB = 0;
                for (size_t k = 0; k < P.size(); k++) { bigD = std::max(bigD, std::fabs(P[k].second)); if (P[k].second < 0) totS += P[k].second; else totB += P[k].second; }
                float top = P.back().first, bot = P.front().first, rng = top - bot > 0 ? top - bot : tkz;
                std::vector<std::pair<float, float>> cur;
                auto closeZ = [&]() {
                    if (cur.size() >= 2) {
                        bool sell = cur[0].second < 0; float lo = cur[0].first, hi = cur.back().first, net = 0;
                        for (size_t k = 0; k < cur.size(); k++) net += cur[k].second;
                        float tot = sell ? totS : totB, share = tot != 0 ? net / tot : 0;
                        bool edge = sell ? hi >= top - 0.25f * rng : lo <= bot + 0.25f * rng;
                        if (share >= 0.40f && edge) {
                            NState ns = classifyNative(lo, hi, net, true);
                            std::string name = sell ? "Dst" : "Acc";
                            DData::DZone z; z.lo = lo; z.hi = hi; z.share = (int)(share * 100 + 0.5f);
                            z.code = !ns.ok ? name + "?" : ns.code == "A" ? "A" : ns.code == "I" ? name : name + "?";
                            z.side = ns.ok ? ns.side : (sell ? "resistance" : "support");
                            if (!ns.ok || ns.code == "A?" || ns.code == "I?") z.side = sell ? "resistance" : "support";
                            Z.push_back(z);
                        }
                    }
                    cur.clear();
                };
                for (size_t k = 0; k < P.size(); k++) {
                    bool heavy = bigD > 0 && std::fabs(P[k].second) >= 0.25f * bigD;
                    if (heavy && !cur.empty() && (P[k].second < 0) == (cur.back().second < 0) && P[k].first - cur.back().first <= 2 * tkz + 1e-6f) cur.push_back(P[k]);
                    else { closeZ(); if (heavy) cur.push_back(P[k]); }
                }
                closeZ();
                std::sort(Z.begin(), Z.end(), [](const DData::DZone& a_, const DData::DZone& b2) { return a_.share > b2.share; });
                if (Z.size() > 2) Z.resize(2);
            }
            D.zones = Z;                                       // native - the file's ZONE lines are not used with live bars
        }
    }
    const std::vector<DData::DNode>& NODES = nat ? natNodes : D.nodes;
    auto nodeOf = [&](int key) -> const DData::DNode* {
        const auto& e = B[key]; const DData::DNode* nb = nullptr;
        for (size_t j = 0; j < NODES.size(); j++) {
            const DData::DNode& n = NODES[j];
            if (n.rk != rk0 || (n.d < 0) != (e.r.d < 0)) continue;
            short yn = yOf(n.px); if (yn < e.t - 1 || yn > e.b + 1) continue;
            if (!nb || std::fabs(n.d) > std::fabs(nb->d)) nb = &n;
        }
        return nb;
    };
    auto hiddenRow = [&](int key) -> bool {
        if (S.showI) return false;
        const DData::DNode* nb = nodeOf(key);
        return nb && !nb->code.empty() && nb->code[0] == 'I';
    };
    std::map<int, bool> lab, labAmt;
    // (2.0.1, Rassul 2026-10-07 11:59 "you have labeled absorption on nodes that are less than 1 when absorption requires high volume")
    // a letter needs a big row: |delta| at least MIN_X times the average row
    for (size_t i = 0, n = 0; i < big.size() && n < 3; i++) { if (hiddenRow(big[i].second)) continue; if (rowX(B[big[i].second].r.d) < MIN_X) break; lab[big[i].second] = true; n++; }
    // (1.9.4, Rassul 23:40 "a pretty big node that's sticking out that doesn't have any ratio ... it looks like it's the biggest
    // node there") GC 23:39: 4168.5 -40 was the biggest row, an I? (sellers' initiative) node, so with the initiative letters off
    // it got neither its amount nor its ratio while smaller rows did. Now the 3 BIGGEST rows always carry their amount, and a
    // hidden initiative node shows its ratio in grey beside it (no letter); the letters still go to the 3 biggest shown nodes.
    for (size_t i = 0; i < big.size() && i < 3; i++) labAmt[big[i].second] = true;
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
        if (S.labels && (lab[kv.first] || labAmt[kv.first]) && r.d != 0) {
            char s[24]; float a = std::fabs(r.d);
            if (a >= 1000) sprintf_s(s, sizeof(s), "%s%.1fk", r.d > 0 ? "+" : "-", a / 1000); else sprintf_s(s, sizeof(s), "%s%.0f", r.d > 0 ? "+" : "-", a);
            short yc = (short)((t + b) / 2);
            bool rightSide = both ? r.d > 0 : dir > 0;
            int tw = textW(s, S.font - 1, false);
            if (rightSide) textLJ((short)(de + 2), yc, s, C_INK, S.font - 1, false);
            else textRJ((short)(de - 2), yc, s, C_INK, S.font - 1, false);
            if (!both) edge = rightSide ? (short)(de + 2 + tw) : (short)(de - 2 - tw);
            if (labAmt[kv.first] && hiddenRow(kv.first)) {             // (1.9.4) the hidden initiative node's ratio, grey, no letter
                const DData::DNode* nb = nodeOf(kv.first);
                float rx = rowX(r.d);
                if (nb && rx > 0) {
                    char xb[16]; if (rx < 10) sprintf_s(xb, sizeof(xb), "%.1fx", rx); else sprintf_s(xb, sizeof(xb), "%.0fx", rx);
                    std::string xs_ = xb; if (xs_.size() > 3 && xs_.substr(xs_.size() - 3) == ".0x") xs_ = xs_.substr(0, xs_.size() - 3) + "x";
                    int xw = textW(xs_.c_str(), S.font - 1, false);
                    if (rightSide) { textLJ((short)(edge + 4), yc, xs_.c_str(), C_MUTED, S.font - 1, false); edge = (short)(edge + 4 + xw); }
                    else { textRJ((short)(edge - 4), yc, xs_.c_str(), C_MUTED, S.font - 1, false); edge = (short)(edge - 4 - xw); }
                }
            }
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
    std::vector<short> letY;                                      // (1.9.2) rows that already carry a letter
    if (S.offlvl) {
        std::string rk = S.range == 1 ? "S" : S.range == 2 ? "D" : S.mins == 30 ? "R" : S.mins == 60 ? "H" :
                         haveN ? "M" + std::to_string(S.mins) : (S.mins > 45 ? "H" : "R");
        std::sort(labd.begin(), labd.end(), [](const LB& p, const LB& q) { return p.t < q.t; });
        short lastY = -1000;
        for (size_t i = 0; i < labd.size(); i++) {
            const DData::DNode* best = nullptr;
            for (size_t j = 0; j < NODES.size(); j++) {
                const DData::DNode& n = NODES[j];
                if (n.rk != rk || (n.d < 0) != (labd[i].d < 0)) continue;
                short yn = yOf(n.px); if (yn < labd[i].t - 1 || yn > labd[i].b + 1) continue;
                if (!best || std::fabs(n.d) > std::fabs(best->d)) best = &n;
            }
            if (!best) continue;
            if (!S.showI && !best->code.empty() && best->code[0] == 'I') continue;   // (1.8.3) initiative only when asked for
            bool inZone = false;                                  // (1.5.3) a node inside a Dst / Acc zone: the zone's label is the read
            for (size_t z = 0; z < D.zones.size(); z++) if (best->px >= D.zones[z].lo - D.tick * 0.5f && best->px <= D.zones[z].hi + D.tick * 0.5f) inZone = true;
            if (inZone) continue;
            COLOR col = best->side == "support" ? 0x0086EFAC : 0x00FCA5A5;
            std::string t = best->code; if (rowX(labd[i].d) > 0) t += xs(rowX(labd[i].d));   // (1.9.5) the drawn row's x
            short y = (short)((labd[i].t + labd[i].b) / 2);
            if (y - lastY < S.font + 2) y = (short)(lastY + S.font + 2);   // two labelled rows touching: stack, never overlap
            lastY = y; letY.push_back(y);
            if (growLeft) textRJ((short)(labd[i].edge - 4), y, t.c_str(), col, S.font, true);    // (1.5.5) right beside its amount
            else textLJ((short)(labd[i].edge + 4), y, t.c_str(), col, S.font, true);
            // (1.8.0, Rassul 12:11 "draws a mark to identify which candle had the absorption ... in choppy price action") a ring on
            // the candle that holds the node's hardest-hitting minute, at the node's price, in the letter's colour
            int bb = D.rings.empty() ? barForMinute(best->tpk) : -1;   // (1.9.1) RING lines draw the rings below
            if (bb >= 0) {
                PNT pp; pp.set(bb, best->px, kBarCenter);
                if (pp.h > pane.left && pp.h < pane.right && pp.v > pane.top + 16 && pp.v < pane.bottom) {
                    // (1.8.2, Rassul 14:23 "use a square for initiative") A / A? = a ring, I / I? = a square
                    bool square = !best->code.empty() && best->code[0] == 'I';
                    if (square) {
                        short l = (short)(pp.h - 5), r_ = (short)(pp.h + 5), t_ = (short)(pp.v - 5), b_ = (short)(pp.v + 5);
                        line(l, t_, r_, t_, col, 2); line(r_, t_, r_, b_, col, 2); line(r_, b_, l, b_, col, 2); line(l, b_, l, t_, col, 2);
                    } else {
                        setPen(col, 2, P_SOLID);
                        CBRUSH hb(col, PAT_HOLLOW); hb.set();
                        RCT rr; rr.set((short)(pp.h - 6), (short)(pp.v - 6), (short)(pp.h + 6), (short)(pp.v + 6));
                        rr.drawOval(DRAW_OPAQUE);
                    }
                }
            }
        }
    }
    // (1.9.1, Rassul 16:31 "i am loosing the circles that were there before ... lets not keep a time constraint for them") the
    // session's rings / squares from the RING lines, whatever Range / minutes this chart shows, on every candle that is on screen
    // (2.2.0, Rassul 2026-10-07 20:15 "there are circle markers where there is no absorption. the signals need to be consistent
    // with the indicator and the absorption signal in the delta profile ... all native irt") a ring is drawn ONLY for a node that
    // carries its letter on the profile right now (same row, same letter, same side), from this chart's own bars - never from the
    // file's RING lines, and never a remembered ring whose row lost its letter. Every circle = an A / A? you can see on the profile.
    if (S.offlvl && nat && !D.rings.empty()) {
        for (size_t i = 0; i < D.rings.size(); i++) {
            const DData::DRing& g = D.rings[i];
            bool square = !g.code.empty() && g.code[0] == 'I';
            if (square && !S.showI) continue;
            {
                short yr0 = yOf(g.px); bool shown = false;
                for (auto& kv : B) {
                    if (yr0 < kv.second.t - 1 || yr0 > kv.second.b + 1) continue;
                    if (!lab[kv.first] || rowX(kv.second.r.d) < MIN_X) continue;
                    const DData::DNode* nb = nodeOf(kv.first);
                    if (nb && nb->code == g.code && nb->side == g.side) { shown = true; break; }
                }
                if (!shown) continue;
            }
            int bb = barForMinute(g.tpk); if (bb < 0) continue;
            PNT pp; pp.set(bb, g.px, kBarCenter);
            if (!(pp.h > pane.left && pp.h < pane.right && pp.v > pane.top + 16 && pp.v < pane.bottom)) continue;
            COLOR col = g.side == "support" ? 0x0086EFAC : 0x00FCA5A5;
            // (1.9.2, Rassul 22:08 / 22:16 "the circles are there" but no A on the profile - "another example of inconsistency")
            // every ring on screen also gets its letter on the profile row at its price, unless that row already has one
            {
                short yr = yOf(g.px);
                for (auto& kv : B) {
                    if (yr < kv.second.t - 1 || yr > kv.second.b + 1) continue;
                    short yc = (short)((kv.second.t + kv.second.b) / 2);
                    bool taken = false; for (size_t q = 0; q < letY.size(); q++) if (std::abs(letY[q] - yc) < S.font + 2) taken = true;
                    if (!taken && rowX(kv.second.r.d) >= MIN_X) {   // (2.0.1) a ring's letter only on a big row
                        std::string lt = g.code; if (rowX(kv.second.r.d) > 0) lt += xs(rowX(kv.second.r.d));   // (1.9.5)
                        short ed = edgeOf.count(kv.first) ? edgeOf[kv.first] : (growLeft ? left : right);
                        if (growLeft) textRJ((short)(ed - 4), yc, lt.c_str(), col, S.font, true);
                        else textLJ((short)(ed + 4), yc, lt.c_str(), col, S.font, true);
                        letY.push_back(yc);
                    }
                    break;
                }
            }
            if (square) {
                short l = (short)(pp.h - 5), r_ = (short)(pp.h + 5), t_ = (short)(pp.v - 5), b_ = (short)(pp.v + 5);
                line(l, t_, r_, t_, col, 2); line(r_, t_, r_, b_, col, 2); line(r_, b_, l, b_, col, 2); line(l, b_, l, t_, col, 2);
            } else {
                setPen(col, 2, P_SOLID);
                CBRUSH hb(col, PAT_HOLLOW); hb.set();
                RCT rr; rr.set((short)(pp.h - 6), (short)(pp.v - 6), (short)(pp.h + 6), (short)(pp.v + 6));
                rr.drawOval(DRAW_OPAQUE);
            }
        }
    }
    if (!D.code.empty() && D.level > 0) {
        bool up = D.side == "support";
        COLOR col = D.code == "Ex" ? C_AMBER : D.code == "In" ? (up ? C_SELL : C_BUY) : (up ? C_BUY : C_SELL);
        std::string t = D.code == "Ab" ? "A" : D.code == "Ex" ? "E" : D.code == "Tr" ? "A" : D.code == "In" ? "I" : D.code.substr(0, 1);
        if (D.strength == "?") t += "?";
        {                                                          // (1.9.5) the key level's x = its drawn row's, like every other
            short yl = yOf(D.level); float dl = 0; const BK* bl = nullptr;
            for (auto& kv : B) if (yl >= kv.second.t - 1 && yl <= kv.second.b + 1 && std::fabs(kv.second.r.d) > std::fabs(dl)) { dl = kv.second.r.d; bl = &kv.second; }
            // (2.0.0) with live bars the level's letter is the native node state of its row (A / A? / I / I?), so it matches
            // the row's bar; the file's level read (E = exhaustion) is kept when the row has no node yet
            if (nat && bl && dl != 0 && D.code != "Ex") {
                NState ns = classifyNative(bl->lo, bl->hi, dl, S.showI);
                if (ns.ok && (S.showI || ns.code[0] != 'I')) { t = ns.code; up = ns.side == "support"; col = ns.code[0] == 'I' ? (up ? C_BUY : C_SELL) : (up ? C_BUY : C_SELL); }
            }
            if (dl != 0 && rowX(dl) > 0) t += xs(rowX(dl)); else if (D.levelX > 0) t += xs(D.levelX);
            // (2.0.2, Rassul 15:10 "you still havent fixed the absorbtion signal") the key level's A / I obeyed no size rule - an
            // A? on a 0.5x row. Absorption / initiative need a big row (MIN_X) like every other letter; E (exhaustion = LOW volume) stays
            if ((t[0] == 'A' || t[0] == 'I') && (dl == 0 || rowX(dl) < MIN_X)) t.clear();
        }
        if (!t.empty()) drawLetter(D.level, t, col);
    }
}

void DeltaProfile::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.status.txt").c_str(), std::ios::trunc);
    if (!f.is_open()) return;
    f << "VERSION," << DLT_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nROWS," << D.rows.size() << "\nASOF," << D.asof
      << "\nRANGE," << drawnRange << "\nDRAWN_ROWS," << drawnRows << "\nBADGE," << D.code << D.strength << "\nWIDTH," << cfg.width << "\nSIDES," << (cfg.sides == 1 ? "Both" : "One") << "\nFACE," << (cfg.face == 0 ? "Left" : "Right") << "\nPLACE," << (cfg.place == 0 ? "Right" : "Left") << "\nSTATE," << what << "\n";
}

// (2.0.0, Rassul 2026-10-07 11:09 "are you building the signals in the delta profile natively? try to build them natively so
// they are aligned with the delta profile") the node rule of lra/delta_profile.node_states, run on THIS chart's bars - the
// same volume at price the bars are drawn from:
//   the node finished forming on the last bar that traded >= 10% of its delta the same way at its prices; after it, price must
//   move MOVE ticks past the node and then not trade back past it (+-TOL ticks) for 15 min of bars. The absorbers' way (a red
//   node = buyers absorbing -> UP) = A, the other way = I (the aggressors won). A held move also needs a candle CLOSED past the
//   node. Until then A? - or I? when the newest bar already traded past it the aggressors' way. peak = the bar that hit hardest.
DeltaProfile::NState DeltaProfile::classifyNative(float lo, float hi, float d, bool showI)
{
    (void)showI;
    NState st; if (liveBars.empty() || d == 0) return st;
    const int MOVE = 4, TOL = 3, HOLD_MIN = 15;
    float tk = liveTick > 0 ? liveTick : (D.tick > 0 ? D.tick : 0.0f);
    auto pxOf = [&](long long pk) -> float { return liveTick > 0 ? (float)(pk * liveTick) : (float)(pk / 100000.0); };
    bool sell = d < 0;
    int nb = (int)liveBars.size(), t0 = -1, peak = -1; float peakD = 0;
    for (int b = 0; b < nb; b++) {
        float bd = 0;
        for (size_t k = 0; k < liveBars[b].d.size(); k++) { float p_ = pxOf(liveBars[b].d[k].first); if (p_ >= lo - tk * 0.01f && p_ <= hi + tk * 0.01f) bd += liveBars[b].d[k].second; }
        if ((bd < 0) == sell && std::fabs(bd) >= 0.1f * std::fabs(d) && bd != 0) { t0 = b; if (std::fabs(bd) > peakD) { peakD = std::fabs(bd); peak = b; } }
    }
    if (t0 < 0) return st;
    st.ok = true; st.peak = liveBars[peak].i;
    int secs = 180;                                             // the bar size, from the bars' own stamps
    if (nb >= 2) { long long dd = (long long)(liveBars[nb - 1].t - liveBars[nb - 2].t); if (dd >= 30 && dd <= 3600) secs = (int)dd; }
    int hold = (HOLD_MIN * 60 + secs - 1) / secs; if (hold < 1) hold = 1;
    // 0 = not yet, 1 = absorbers' way held (A), 2 = the other way held (I)
    auto resolve = [&](int from, int& at) -> int {
        for (int k = from; k < nb; k++) {
            bool upGo = liveBars[k].h >= hi + MOVE * tk, dnGo = liveBars[k].l <= lo - MOVE * tk;
            for (int pass = 0; pass < 2; pass++) {
                bool goUp = pass == 0; if (goUp ? !upGo : !dnGo) continue;
                if (k + hold >= nb) return 0;                   // not long enough to know yet
                bool held = true;
                for (int z = k + 1; z <= k + hold; z++) { if (goUp ? liveBars[z].l < hi - TOL * tk : liveBars[z].h > lo + TOL * tk) { held = false; break; } }
                if (held) { at = k + hold; return goUp == sell ? 1 : 2; }
            }
        }
        return 0;
    };
    int at = -1, r = resolve(t0 + 1, at);
    if (r == 1) { int at2 = -1; if (resolve(at + 1, at2) == 2) { r = 2; at = at2; } }   // a held node can still fail later
    if (r != 0) {                                               // ... and a candle CLOSED on the winning side
        bool up = (r == 1) == sell, closed = false;
        for (int k = t0 + 1; k < nb; k++) if (up ? liveBars[k].c > hi : liveBars[k].c < lo) { closed = true; break; }
        if (!closed) r = 0;
    }
    bool wonUp = sell == (r != 2);
    st.code = r == 0 ? "A?" : r == 1 ? "A" : "I";
    if (r == 0 && t0 + 1 < nb) {                                // leaning: the newest bar traded past it the aggressors' way
        const LBar& lc = liveBars[nb - 1];
        bool aggUp = !sell;
        if (aggUp ? lc.h > hi + tk * 0.5f : lc.l < lo - tk * 0.5f) { st.code = "I?"; wonUp = aggUp; }
    }
    st.side = wonUp ? "support" : "resistance";
    return st;
}

std::string DeltaProfile::stampOf(int bar)
{
    RTARRAYI dt(barDateTime); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[bar], &t);
    char b[24]; sprintf_s(b, sizeof(b), "%04d-%02d-%02d %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
    return b;
}

// (1.9.0) the profile from the chart's own bars' volume at price: Last N minutes (bars whose close is within N minutes of the
// newest bar's close, the forming bar included), Session (from 17:00 CT) or Day (from 08:30). Recomputed only when the newest
// bar's volume or the bar count changed.
// (1.9.1, IRT closed itself at 16:36 the first time 1.9.0 ran on every chart) reading one bar's volume at price is guarded:
// only the price rows the bar reports are read (1.9.0 asked for 400 rows when a bar reported none - past the end of IRT's data),
// and a fault inside IRT's volume-at-price turns the live bars OFF for this chart (the file's rows are drawn) instead of
// taking IRT down. Plain C types only in here (structured exception handling).
int DeltaProfile::readBarVap(RTARRAYP* VP, int i, VOLPROFILE* out, int cap)
{
#ifdef _MSC_VER
    __try {
#endif
        BARSTATISTICS bs; memset(&bs, 0, sizeof(bs));
        if (VP->getBarStatistics(i, bs) != RTX_OK) return -2;
        int np = bs.prices; if (np <= 0) return 0; if (np > cap) np = cap;
        int k = 0;
        for (; k < np; k++) { memset(&out[k], 0, sizeof(VOLPROFILE)); if (VP->getVolumeProfile(i, k, out[k]) != RTX_OK) break; }
        return k;
#ifdef _MSC_VER
    } __except (1) { return -1; }
#endif
}

void DeltaProfile::trace(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.trace.txt";
    bool big = dl::fileStamp(p) > 0 && [&]() { std::ifstream f(p.c_str(), std::ios::ate | std::ios::binary); return (long long)f.tellg() > 200000; }();
    std::ofstream f(p.c_str(), big ? std::ios::trunc : std::ios::app);
    time_t now = time(nullptr); struct tm t; localtime_s(&t, &now);
    char b[32]; strftime(b, sizeof(b), "%m-%d %H:%M:%S", &t);
    f << b << " " << DLT_VERSION << " " << root << " " << what << "\n";
}

// (1.9.0) the profile from the chart's own bars' volume at price: Last N minutes (bars whose close is within N minutes of the
// newest bar's close, the forming bar included), Session (from 17:00 CT) or Day (from 08:30). (1.9.1) Recomputed at most once
// a second per chart (or when a bar is added / the settings change), never after a fault.
bool DeltaProfile::buildLive(const DSet& S)
{
    if (liveOff) return false;
    long n = getBarCount(); if (n < 2) return false;
    RTARRAYI dt(barDateTime);
    RTARRAYI vo(barVolume);
    long key = n * 1000003L + (long)vo[(int)n - 1];
    long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    bool same = root == liveRoot && S.range == liveRange && S.mins == liveMins;
    if (same && (key == liveKey || (key / 1000003L == liveKey / 1000003L && nowMs - liveAt < 1000))) return !liveRows.empty();
    RTARRAYP VP(barVolumeProfile);
    struct tm tl; memset(&tl, 0, sizeof(tl)); getLocaltime((RTDATE)dt[(int)n - 1], &tl); tl.tm_isdst = -1;
    time_t tLast = mktime(&tl);
    time_t tFrom;
    if (S.range == 0) tFrom = tLast - (time_t)S.mins * 60;
    else {
        struct tm a = tl; a.tm_sec = 0;
        if (S.range == 1) { a.tm_hour = 17; a.tm_min = 0; if (tl.tm_hour < 17) a.tm_mday -= 1; }      // the session opened 17:00
        else { a.tm_hour = 8; a.tm_min = 30; if (tl.tm_hour >= 17) a.tm_mday += 1; }                   // the day from 08:30
        a.tm_isdst = -1; tFrom = mktime(&a);
    }
    bool first = liveKey == -1;
    if (first) trace("live: first build");
    std::map<long long, DRow> agg;
    // (2.1.0) the tick size from IRT's own symbol data (the file's TICK only as a fallback)
    float tk = getProperty(SYM_TICKINCR); if (!(tk > 0) || tk > 1000) tk = D.tick > 0 ? D.tick : 0;
    if (D.tick > 0 && std::fabs(tk - D.tick) > D.tick * 0.01f) tk = D.tick;   // a disagreement: trust the file's (checked) tick
    std::vector<LBar> bars;
    RTARRAY ao(barOpen), ah(barHigh), al(barLow), ac(barClose);
    static VOLPROFILE buf[4000];
    int from = (int)n - 3000; if (from < 0) from = 0;
    for (int i = (int)n - 1; i >= from; i--) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t); t.tm_isdst = -1;
        if (mktime(&t) <= tFrom) break;                                  // bars are stamped at their close
        int np = readBarVap(&VP, i, buf, 4000);
        if (np == -1) {                                                  // a fault inside IRT: stop using live bars on this chart
            char b[96]; sprintf_s(b, sizeof(b), "live: FAULT reading bar %d of %ld - live bars off, drawing the file", i, n); trace(b);
            liveOff = true; liveRows.clear(); return false;
        }
        if (np == -2) { if (i == (int)n - 1) continue; liveRows.clear(); liveKey = key; liveAt = nowMs; liveRoot = root; liveRange = S.range; liveMins = S.mins; return false; }
        LBar lb; lb.i = i; lb.t = mktime(&t); lb.o = ao[i]; lb.h = ah[i]; lb.l = al[i]; lb.c = ac[i];
        for (int k = 0; k < np; k++) {
            const VOLPROFILE& vp = buf[k];
            if (vp.totalVolume <= 0 && vp.buyVolume <= 0 && vp.sellVolume <= 0) continue;
            long long pk = tk > 0 ? (long long)std::floor(vp.price / tk + 0.5) : (long long)std::floor(vp.price * 100000.0 + 0.5);
            DRow& r = agg[pk]; r.px = tk > 0 ? (float)(pk * tk) : vp.price; r.d += (float)(vp.buyVolume - vp.sellVolume); r.v += (float)vp.totalVolume;
            float bd = (float)(vp.buyVolume - vp.sellVolume); if (bd != 0) lb.d.push_back(std::make_pair(pk, bd));
        }
        bars.push_back(lb);
    }
    liveRows.clear();
    for (auto& kv : agg) liveRows.push_back(kv.second);
    std::reverse(bars.begin(), bars.end()); liveBars.swap(bars); liveTick = tk;
    liveKey = key; liveAt = nowMs; liveRoot = root; liveRange = S.range; liveMins = S.mins;
    if (first) { char b[64]; sprintf_s(b, sizeof(b), "live: ok, %d rows", (int)liveRows.size()); trace(b); }
    return !liveRows.empty();
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
    // IRT stamps a bar by its CLOSE (the 08:36-08:39 bar is "08:39"); a minute is stamped by its end too (08:39 = 08:38-08:39).
    // The minute's bar = the first bar whose stamp is at or after the minute's stamp. (1.9.1) the stamps of the last 3000 bars
    // are kept (rebuilt when a bar is added) and searched, so 40 rings cost one pass, not 40.
    int y, mo, d, h, mi;
    if (tpk.size() < 16 || sscanf_s(tpk.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5) return -1;
    long n = getBarCount(); if (n < 1) return -1;
    RTARRAYI dt(barDateTime);
    long key = n * 7919L + (long)(dt[(int)n - 1] % 100000);
    int from = (int)n - 3000; if (from < 0) from = 0;
    if (key != barStampKey || (int)barStamp.size() != (int)n - from) {
        barStamp.assign((size_t)((int)n - from), 0);
        for (int i = from; i < (int)n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
            barStamp[(size_t)(i - from)] = ((((t.tm_year + 1900LL) * 100 + t.tm_mon + 1) * 100 + t.tm_mday) * 10000LL + t.tm_hour * 100 + t.tm_min) * 100 + t.tm_sec;
        }
        barStampKey = key;
    }
    long long e = (((((long long)y * 100 + mo) * 100 + d) * 10000LL) + h * 100 + mi) * 100;
    auto it = std::lower_bound(barStamp.begin(), barStamp.end(), e);
    if (it == barStamp.end()) return -1;
    if (it == barStamp.begin() && from > 0) return -1;              // older than the bars kept
    return from + (int)(it - barStamp.begin());
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
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);   // (1.9.0) the chart's volume at price
    p->setDescription("LRA Delta Profile (DLT)");
    p->setVersion(DLT_VERSION);   // (1.7.0) the settings window showed 1.4.0
    return p;
}
