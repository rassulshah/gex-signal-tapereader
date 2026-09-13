/********************************************************************************
 *  GammaProfile.cpp  —  Investor/RT RTX extension   (IRT gamma profile, Phase 1)
 *
 *  Draws a horizontal per-strike gamma histogram + a reference-level rail on the
 *  price pane, aligned to the instrument's own price axis (INSTRUMENT_SCALE), in
 *  a reserved strip at the right edge — the price bars are never overlapped.
 *
 *  Reads:  %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv
 *  (the same folder FlexLevels already exports to; the panel will write this
 *   file in Phase 0. A sample file lets it draw before the panel side is wired.)
 *
 *  CSV format (one record per line, prices in the CHART's price space; '#'=comment):
 *     STRIKE,<price>,<pctKing -100..100>,<rank>,<isKing 0|1>
 *     KING,<price>     CW,<price>    PW,<price>
 *     FLIP,<price>     EMH,<price>   EML,<price>
 *
 *  Structure mirrors LinnSoft's lsSampleRTX.cpp:
 *   - init/setup/calc/done/destroy are defined as cppExtension:: members (the base
 *     class declares them with NO body, so the DLL must supply them).
 *   - only draw() is overridden in the derived class (it has a default base body).
 *   - CreateExtension() news the class, sets flags/description/version, returns it.
 *     (Do NOT define pExtension — the SDK lib owns it.)
 *
 *  Build: x64 Release DLL, link  sdk\c++\lib\irtsdkV143-x64.lib  (VS2022 / v143).
 *  Install: copy GammaProfile.dll into  C:\Program Files\LinnSoft\InvestorRT\dllx64\
 ********************************************************************************/
#include "irtsdk.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>

// ---- palette (0x00RRGGBB) -------------------------------------------------
// Diverging heatmap to match the Skylit tape: green at ~0, gold at strong +gamma,
// magenta at strong -gamma. The King is colored by its own polarity (not a special
// color) exactly like the tape, and stays distinct as the largest bar + KING label.
static const COLOR C_GREEN  = 0x00179E5C;   // gamma ~ 0 (tape midpoint)
static const COLOR C_GOLD   = 0x00E3C341;   // strong +gamma (tape yellow)
static const COLOR C_MAGENTA= 0x00C43BAF;   // strong -gamma (tape magenta)
static const COLOR C_PURPLE = 0x00A86FE0;   // (legacy) -gamma node
static const COLOR C_KING   = 0x00FFF0A6;   // (legacy) King / POC
static const COLOR C_INK    = 0x00202020;   // label ink on bright bars
static const COLOR C_PINK   = 0x00FF5DB0;   // walls
static const COLOR C_WHITE  = 0x00E6EDF5;   // flip
static const COLOR C_CYAN   = 0x004FD0E0;   // expected-move band

static const short MARGIN = 130;            // px width of the reserved histogram strip

// linear blend of two 0x00RRGGBB colors, t in [0,1]
static COLOR lerpColor(COLOR a, COLOR b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
    int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
    int r=(int)(ar+(br-ar)*t+0.5f), g=(int)(ag+(bg-ag)*t+0.5f), bl=(int)(ab+(bb-ab)*t+0.5f);
    return (COLOR)(((COLOR)r<<16) | ((COLOR)g<<8) | (COLOR)bl);
}
// diverging heatmap by signed %King: 0 -> green, +100 -> gold, -100 -> magenta
static COLOR nodeColor(float pct) {
    float t = (pct < 0 ? -pct : pct) / 100.0f;
    return (pct >= 0) ? lerpColor(C_GREEN, C_GOLD, t) : lerpColor(C_GREEN, C_MAGENTA, t);
}

struct GStrike { float price; float pct; int rank; bool king; };

// ---------------------------------------------------------------------------
class GammaProfile : public cppExtension {
public:
    GammaProfile() : cppExtension() {}
    virtual int draw(void);                 // only draw() is overridden

    std::vector<GStrike> strikes;
    float lvl[6];                           // KING, CW, PW, FLIP, EMH, EML
    bool  has[6];

    void load();
    void render();
    void drawLevel(int lastBar, short right, int idx, COLOR col, const char* label, bool above);
};

// ---- base-vtable resolvers (required: no default body in the SDK base) -----
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }   // we draw manually; arrays unused
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

// ---- data load ------------------------------------------------------------
void GammaProfile::load()
{
    for (int i = 0; i < 6; i++) { has[i] = false; lvl[i] = 0.0f; }

    std::string path;
    const char* up = getenv("USERPROFILE");
    if (!up) return;
    path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\GammaProfile.csv";

    std::ifstream f(path.c_str());
    if (!f.is_open()) { strikes.clear(); return; }

    std::vector<GStrike> tmp;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> t;
        std::stringstream ss(line);
        std::string it;
        while (std::getline(ss, it, ',')) t.push_back(it);
        if (t.empty()) continue;

        if (t[0] == "STRIKE" && t.size() >= 5) {
            GStrike s;
            s.price = (float)atof(t[1].c_str());
            s.pct   = (float)atof(t[2].c_str());
            s.rank  = atoi(t[3].c_str());
            s.king  = (atoi(t[4].c_str()) != 0);
            tmp.push_back(s);
        } else if (t.size() >= 2) {
            float v = (float)atof(t[1].c_str());
            if      (t[0] == "KING") { lvl[0] = v; has[0] = true; }
            else if (t[0] == "CW")   { lvl[1] = v; has[1] = true; }
            else if (t[0] == "PW")   { lvl[2] = v; has[2] = true; }
            else if (t[0] == "FLIP") { lvl[3] = v; has[3] = true; }
            else if (t[0] == "EMH")  { lvl[4] = v; has[4] = true; }
            else if (t[0] == "EML")  { lvl[5] = v; has[5] = true; }
        }
    }
    strikes.swap(tmp);
}

// ---- render ---------------------------------------------------------------
void GammaProfile::render()
{
    if (strikes.empty()) return;
    long n = getBarCount();
    if (n < 2) return;
    int lastBar = (int)n - 1;

    RCT pane;
    pane.getPaneRect(false);                // drawing area, excludes the price scale
    short right = pane.right;

    // bar height from the on-screen pixel gap between two adjacent strikes
    short barH = 6;
    if (strikes.size() >= 2) {
        PNT a; a.set(lastBar, strikes[0].price);
        PNT b; b.set(lastBar, strikes[1].price);
        short d = (short)std::abs((int)b.v - (int)a.v);
        if (d > 3) barH = (short)(d * 0.8f);
    }

    // histogram: bars in the reserved right strip, growing left toward price
    for (size_t i = 0; i < strikes.size(); i++) {
        GStrike& s = strikes[i];
        PNT p; p.set(lastBar, s.price);
        short len = (short)(std::fabs(s.pct) / 100.0f * MARGIN);
        if (len < 3) len = 3;
        COLOR col = nodeColor(s.pct);   // polarity+magnitude heatmap, matches the tape

        RCT bar;
        bar.set((short)(right - len), (short)(p.v - barH / 2),
                right,                 (short)(p.v + barH / 2));   // left,top,right,bottom
        bar.draw(0, col, col, DRAW_OPAQUE, PAT_SOLID);

        // rank (inside-left) and %King (inside-right) as small labels
        char buf[32];
        if (s.rank >= 1 && s.rank <= 5) {
            sprintf_s(buf, sizeof(buf), "%d", s.rank);
            PNT t; t.set(lastBar, s.price); t.h = (short)(right - len + 3); t.v = (short)(p.v - 6);
            t.drawText(buf, -1, C_INK);
        }
        sprintf_s(buf, sizeof(buf), "%d%%", (int)(s.pct + (s.pct >= 0 ? 0.5f : -0.5f)));
        PNT q; q.set(lastBar, s.price); q.h = (short)(right - 30); q.v = (short)(p.v - 6);
        q.drawText(buf, -1, C_INK);
    }

    // reference-level rail (labels sit ABOVE the line)
    drawLevel(lastBar, right, 0, C_KING,  "KING",      true);
    drawLevel(lastBar, right, 1, C_PINK,  "CALL WALL", true);
    drawLevel(lastBar, right, 2, C_PINK,  "PUT WALL",  false);
    drawLevel(lastBar, right, 3, C_WHITE, "FLIP",      true);
    drawLevel(lastBar, right, 4, C_CYAN,  "EM-H",      true);
    drawLevel(lastBar, right, 5, C_CYAN,  "EM-L",      false);
}

void GammaProfile::drawLevel(int lastBar, short right, int idx, COLOR col, const char* label, bool above)
{
    if (!has[idx]) return;
    setPen(col, 1, P_SOLID);
    PNT a; a.set(0, lvl[idx]);        a.setDrawPosition();
    PNT b; b.set(lastBar, lvl[idx]);  b.drawLineTo();

    PNT t; t.set(lastBar, lvl[idx]);
    t.h = (short)(right - MARGIN - 74);         // to the left of the histogram strip
    t.v = (short)(t.v + (above ? -11 : 3));     // label above (or below) the line, never on it
    t.drawText(label, -1, col);
}

// ---- draw() override: reload the tiny file, then paint --------------------
int GammaProfile::draw(void)
{
    load();
    render();
    return RTX_OK;
}

// ---- required factory -----------------------------------------------------
extern "C" cppExtension *CreateExtension(void)
{
    GammaProfile *p = new GammaProfile();
    p->setArrayCount(1);                        // no output arrays are drawn (NO_UI)
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | NO_PARMS | INSTRUMENT_SCALE);
    p->setDescription("IRT Gamma Profile \x97 per-strike gamma histogram + level rail (reads lsFlexLevels\\GammaProfile.csv)");
    p->setVersion("0.1");
    return p;
}
