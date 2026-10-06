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

static const char* DLT_VERSION = "1.1.5";
static const COLOR C_BUY   = 0x0022C55E;
static const COLOR C_SELL  = 0x00EF4444;
static const COLOR C_AMBER = 0x00F59E0B;
static const COLOR C_VOL   = 0x00243040;   // faint volume bar over the black chart
static const COLOR C_AXIS  = 0x00334155;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;

struct DIdx { int market, width, gap, font, labels, place, range, group, sides; };
static DIdx DX;
struct DSet { int market = 0, width = 80, gap = 8, font = 9, place = 1, range = 0, group = 1, sides = 0; bool labels = true; };

struct DRow { float px = 0, d = 0, v = 0; };
struct DData {
    bool ok = false; std::string asof, from, to, side, name, code, strength; float level = 0;
    std::vector<DRow> rows, srows, drows, hrows; float tick = 0;
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
int DeltaProfile::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DeltaProfile::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DeltaProfile::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(3);   // (1.1.3) BUMPED: 1.1.x added Place / Range / Ticks per row / Sides - charts saved with the 1.0 settings crashed IRT when the dialog opened (msglog 19:05); resets to the defaults
    setParameterDialogHeight(8);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    DX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    DX.width  = pc++; setIntegerParameter("Width px", 80, NUMW, SL);
    DX.gap    = pc++; setIntegerParameter("Gap px", 8, NUMW);
    DX.font   = pc++; setIntegerParameter("Font size (pt)", 9, NUMW, SL);
    DX.labels = pc++; setBoolParameter("Values on the biggest bars", true);
    DX.place  = pc++; setListParameter("Place", 0, "Left;Right");   // (1.1.4) Left first: the list default is its first entry
    DX.range  = pc++; setListParameter("Range", 0, "Last 30 min;Last 60 min;Session;Day from 08:30");   // (1.1.0) as the examples Rassul sent
    DX.group  = pc++; setIntegerParameter("Ticks per row (0 = auto)", 1, NUMW);
    DX.sides  = pc++; setListParameter("Sides", 0, "One;Both");   // (1.0.3) Left = at the chart's left edge, apart from the Dealer Profile
    return RTX_OK;
}

void DeltaProfile::readSettings(DSet& S)
{
    S.market = getListIndex(DX.market);
    S.width = getIntegerValue(DX.width); if (S.width < 30) S.width = 80; if (S.width > 600) S.width = 600;
    S.gap = getIntegerValue(DX.gap); if (S.gap < -300) S.gap = -300; if (S.gap > 400) S.gap = 400;   // (1.0.1) negative = closer to / over the Dealer Profile
    S.font = getIntegerValue(DX.font); if (S.font < 6) S.font = 9; if (S.font > 18) S.font = 18;
    S.labels = isBoxChecked(DX.labels) != 0;
    { int pi = getListIndex(DX.place); S.place = (pi == 1) ? 0 : 1; }   // (1.1.4) list Left;Right -> place 1 = Left, 0 = Right
    S.range = getListIndex(DX.range); if (S.range < 0 || S.range > 3) S.range = 0;
    S.group = getIntegerValue(DX.group); if (S.group < 0) S.group = 0; if (S.group > 500) S.group = 500;
    S.sides = getListIndex(DX.sides); if (S.sides < 0 || S.sides > 1) S.sides = 0;
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
    short right, left;
    if (S.place == 1) {                                             // (1.0.3) Left: from the pane's left edge + gap
        left = (short)(pane.left + 4 + (S.gap > 0 ? S.gap : 0)); right = (short)(left + S.width);
        if (right >= paneR - 40) return;
    } else {                                                        // Right: just left of the Dealer Profile
        right = (short)(paneR - 2 - dealerReach() - S.gap); left = (short)(right - S.width);
        if (left <= pane.left + 40) return;
    }
    // (1.0.3, Rassul 17:52) red and green both grow TOWARD the price chart from one base line on the far side
    int dir = S.place == 1 ? 1 : -1;                                // Left: grow right; Right: grow left
    short base = S.place == 1 ? left : right;
    if (S.place == 1) textLJ((short)(left + 2), (short)(pane.top + 8), "DLT", C_MUTED, 8, true);
    else textRJ((short)(right - 2), (short)(pane.top + 8), "DLT", C_MUTED, 8, true);
    if (S.sides != 1) line(base, (short)(pane.top + 16), base, pane.bottom, C_AXIS, 1);
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
    // the behaviour code at the level (below a support, above a resistance)
    if (!D.code.empty() && D.level > 0) {
        bool up = D.side == "support";
        COLOR c = D.code == "Ex" ? C_AMBER : D.code == "In" ? (up ? C_SELL : C_BUY) : (up ? C_BUY : C_SELL);
        std::string t = D.code + (D.strength == "?" ? "?" : D.strength == "+" ? "+" : "");   // (1.1.2) ? = setting up, outlined; check = confirmed, solid
        bool tick = D.strength == "v";
        int tw = textW(t.c_str(), S.font, true) + (tick ? 9 : 0);
        short y = yOf(D.level), h = (short)(S.font + 5);
        short top = up ? (short)(y + 3) : (short)(y - 3 - h);
        short l = (short)(mid - tw / 2 - 4), r = (short)(mid + tw / 2 + 4);
        RCT bg; bg.set(l, top, r, (short)(top + h));
        bool solid = tick;                                          // confirmed = solid, else outlined
        bg.draw(1, c, solid ? c : C_DARK, DRAW_OPAQUE, PAT_SOLID);
        COLOR ink = solid ? C_DARK : c;
        textLJ((short)(l + 4), (short)(top + h / 2), t.c_str(), ink, S.font, true);
        if (tick) {                                                 // a drawn check mark (fonts may lack the glyph)
            short cx = (short)(r - 11), cy = (short)(top + h / 2);
            line(cx, cy, (short)(cx + 3), (short)(cy + 3), ink, 2);
            line((short)(cx + 3), (short)(cy + 3), (short)(cx + 8), (short)(cy - 3), ink, 2);
        }
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
    if (dialogReady()) readSettings(cfg);       // this chart's own settings (one object serves every chart)
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
    p->setVersion("1.1.5");
    return p;
}
