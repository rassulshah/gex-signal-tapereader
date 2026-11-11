#include <filesystem>
// test_dealerprofile_2610.cpp -- 2.61.0: the bright stretches are recorded (<MKT>-liqlines-<session>.csv). Built on test_dealerprofile_2600.cpp.
// test_dealerprofile_2600.cpp -- 2.60.0: LIQUIDITY ON THE LEVEL LINES (a level line is bright, 2 px, side colour, over the candles where a node carried its badge; no number). Built on test_dealerprofile_2590.cpp.
// test_dealerprofile_2590.cpp -- lsDealerProfile / lsLimitProfile 2.59.0 (2026-11-10): THE NODE PROFILE. The native top book (kind G) is drawn as nodes anchored at the
// column's outer edge pointing at price, text INSIDE from the tip ([level badge] lots  pct%  time [S20 / ? / EXIT]), rank outside with no box, a level within
// 1 ATR names the largest node only, the nodes change once a minute (at once on a touch / cold book). Built on test_dealerprofile_2560.cpp (its checks kept).
// (2.52.0 update: the liquidity rank box at the row FRONT - two checks follow DP_LIQ_RANK_FRONT; the version / HELD checks were already failing before 2.52.0)
// test_dealerprofile_2560.cpp -- lsDealerProfile 2.56.0: the ENT / EXIT tags on the Key Levels rows (EntryBadge.h, lsBreakout fact kind E): an ES Pdoc
// label draws "ENT Pdoc" on a PB POC row INSIDE the Key Levels column (no new column, nothing on the price area), status ENTRY_BADGE; a Pb gets
// no tag; no E producer -> no tag. Built on test_dealerprofile_2550.cpp -- lsDealerProfile 2.55.0 ("NN% real" on the rows when the model is LIVE) + 2.54.0: the NATIVE TOP BOOK (SignalBoard kind G from lsBookFlow 1.37.0) in the Liquidity column; harness from test_dealerprofile_2400.cpp
// (was) test_dealerprofile_2400.cpp -- lsDealerProfile 2.40.0 (2026-11-01, Rassul 09:49-10:21 CT, mockup lp_proposal.html) through the REAL DealerProfile.cpp:
// the Liquidity column = the WHOLE BOOK (B code 4 from lsLiquidityProfile 0.70.0): top 5 + level-linked nodes >= 50% of the king with their real
// ranks (6, 7, ...) and the level names; "lots  pct%" = % of the king; K box on the king; no STAYS / GONE tags; the Key Levels column on the RIGHT,
// just left of the Liquidity column (one published Levels width), badges left-justified, no odds %, no rank chips; the left keep-out cleared.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2400.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// The mocked host below is test_dealerprofile_2390.cpp's (unchanged).
// test_dealerprofile_290.cpp -- Dealer Profile 2.9.0 (Rassul approved 2026-10-13 18:05): the SDK-free logic (DealerProfileAuditLogic.h)
// and the REAL DealerProfile.cpp draw() on a mocked host (irtsdk_host: real irtsdk.h, the drawing calls recorded), fed with the
// real ES files of 13 Oct (tests_dp290/: LRA-Dealer-ES.csv = bridge file 2.4 on the 13 Oct tables, MenthorQLevels.csv, key levels,
// the 3-min bars cut at 2:57 PM CT).
// Build (Linux, from plugin/):
//   g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_290.cpp -Wl,--unresolved-symbols=ignore-all -o t
//   ./t                      the checks
//   ./t ops.txt [dir] [HH:MM] [mouse strike]   also dump every draw call for the PNG render check (render_check.py)
#include "DealerRead_test_linux_sdk_compat.h"
#include <ctime>
#include <cstring>
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif
static inline int localtime_s(struct tm* r, const time_t* t) { return localtime_r(t, r) ? 0 : 1; }
static inline int strncat_s(char* d, size_t n, const char* s, size_t) { size_t l = strlen(d); if (l + 1 >= n) return 0; strncat(d, s, n - l - 1); return 0; }
static inline int strncpy_s(char* d, size_t n, const char* s, size_t) { if (!n) return 1; strncpy(d, s, n - 1); d[n - 1] = 0; return 0; }
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
#include <cmath>
#define private public
#define protected public
#include "DealerProfile.cpp"
#include <dirent.h>
#undef private
#undef protected
#include <cstdio>
#include <sys/stat.h>

static int FAILS = 0, PASSES = 0;
static void check(bool c, const std::string& m) { if (c) ++PASSES; else { ++FAILS; std::printf("FAIL %s\n", m.c_str()); } }

// ------------------------------------------------------------------------------------------------ the mocked host (records draws)
struct Chart { std::string root, sym; int spb = 180; void* user = nullptr;
               std::vector<unsigned long> dt; std::vector<float> o, h, l, c; std::vector<unsigned long> v; };
static Chart* CUR = nullptr;
static float PTOP = 7935.0f, PBOT = 7795.0f; static const int PH = 820;
static short yOfP(float p) { return (short)std::lround((PTOP - p) * PH / (PTOP - PBOT)); }
static float pOfY(int y) { return PTOP - (float)y * (PTOP - PBOT) / PH; }
static const int X0 = 200, PPB = 5;
static RTDATE NOW = 0;
static COLOR pen = 0, ink = 0, brush = 0; static int penW = 1; static int curFont = 10; static bool curBold = false;
static short posH = 0, posV = 0;
static int penStyle = 0;
struct Op { std::string kind; int l, t, r, b; unsigned long col, col2; int font; bool bold; std::string text; int style; };
static std::vector<Op> OPS;
#include "tests_dp290/dp_widths.h"   // Liberation Sans advance widths (Helvetica metrics), 6-14 pt - the render check's font
static int tw(const char* s) { int pt = curFont < 6 ? 6 : curFont > 14 ? 14 : curFont; int w = 0;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) w += (*p >= 32 && *p < 127) ? DP_W[curBold ? 1 : 0][pt - 6][*p - 32] : pt / 2; return w; }
void* dp_test_cursor_wnd = nullptr;

cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
long cppExtension::getBarCount(void) { return (long)CUR->dt.size(); }
char* cppExtension::getSymbol(void) { return (char*)CUR->sym.c_str(); }
int cppExtension::getSecondsPerBar(void) { return CUR->spb; }
struct tm* cppExtension::getLocaltime(RTDATE d, struct tm* p) { time_t t = (time_t)d; gmtime_r(&t, p); return p; }
void* cppExtension::getUserData(void) { return CUR->user; }
void cppExtension::setUserData(void* p) { CUR->user = p; }
cppExtension::RTARRAY::RTARRAY(fARRAY f) : tempFloat(0), bFreeTempArray(0), pArray(&tempArray), fArray(f), count(0)
{
    tempArray.data = nullptr; tempArray.size = 0; tempArray.width = 0; tempArray.element = 0;
    std::vector<float>* src = f == barOpen ? &CUR->o : f == barHigh ? &CUR->h : f == barLow ? &CUR->l : f == barClose ? &CUR->c : nullptr;
    if (src) { tempArray.data = src->data(); count = (long)src->size(); tempArray.size = count; }
}
cppExtension::RTARRAY::~RTARRAY() {}
float& cppExtension::RTARRAY::operator[](int i) { if (i >= 0 && i < count && pArray->data) return pArray->data[i]; return tempFloat; }
cppExtension::RTARRAYI::RTARRAYI(iARRAY a) : tempULong(0), pArray(&tempArrayI), iArray(a), count(0)
{
    tempArrayI.data = nullptr; tempArrayI.size = 0;
    std::vector<unsigned long>* src = a == barDateTime ? &CUR->dt : a == barVolume ? &CUR->v : nullptr;
    if (src) { tempArrayI.data = src->data(); count = (long)src->size(); }
}
cppExtension::RTARRAYI::~RTARRAYI() {}
unsigned long& cppExtension::RTARRAYI::operator[](int i) { if (i >= 0 && i < count) return pArray->data[i]; return tempULong; }
RTX_RESULT cppExtension::RTARRAY::getChartIndicator(const char*, int, int) { return RTX_FAIL; }
RTX_RESULT cppExtension::RTARRAY::getCustomIndicator(char*, int, RTBOOL, const char*, int) { return RTX_FAIL; }
cppExtension::FONT::FONT(FONT_ID i, short s, FONT_STYLE st) : id(i), size(s), style(st) {}
char* cppExtension::getRootSymbol(char* b, const char*) { if (b) { strcpy(b, CUR->root.c_str()); return b; } return (char*)CUR->root.c_str(); }
static std::string PERIOD = "3 Minutes";
char* cppExtension::getPeriodicityLabel(void) { return (char*)PERIOD.c_str(); }
RTX_RESULT cppExtension::getVisibleBars(int* s, int* e) { *s = 0; *e = (int)CUR->dt.size() - 1; return RTX_OK; }
void cppExtension::setPen(COLOR c, short w, PEN_STYLE st) { pen = c; penW = w; penStyle = (int)st; }
void cppExtension::setTextColor(COLOR c) { ink = c; }
RTX_RESULT cppExtension::setFont(FONT& f) { curFont = f.size; curBold = f.style == BOLD; return RTX_OK; }
int cppExtension::getTextWidth(const char* s, int) { return tw(s); }
void cppExtension::getFontMetrics(int* lead, int* asc, int* desc) { *lead = 0; *asc = (int)std::lround(curFont * 4.0 / 3 * 0.905); *desc = (int)std::lround(curFont * 4.0 / 3 * 0.212); }
double cppExtension::getTickIncrement(const char*) { return 0.25; }
RTDATE cppExtension::currentDate(void) { return NOW; }
static int MOCK_LIST_GET[32] = { -1 };
int cppExtension::getListIndex(int i) { return i >= 0 && i < 32 ? MOCK_LIST_GET[i] : -1; }
RTX_RESULT cppExtension::invalidateChart(RTBOOL) { return RTX_OK; }
cppExtension::CBRUSH::CBRUSH(COLOR c, BRUSH_STYLE s) : color(c), style(s) {}
void cppExtension::CBRUSH::set() { brush = color; }
static int PW = 1600;   // (2110) the pane width, changed per test
void cppExtension::RCT::getPaneRect(RTBOOL) { left = 0; top = 0; right = (short)PW; bottom = PH; }
void cppExtension::RCT::getScaleRect(void) { left = (short)PW; top = 0; right = (short)(PW + 60); bottom = PH; }
void cppExtension::RCT::set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
void cppExtension::RCT::draw(short w, COLOR border, COLOR fill, DRAWTYPE, BRUSH_STYLE bs) { OPS.push_back(Op{"box", left, top, right, bottom, fill & 0xFFFFFF, border & 0xFFFFFF, w, false, "", (int)bs}); }
void cppExtension::RCT::drawRounded(int, int) { OPS.push_back(Op{"rbox", left, top, right, bottom, brush & 0xFFFFFF, pen & 0xFFFFFF, penW, false, "", penStyle}); }
void cppExtension::RCT::drawText(const char* t, RTBOOL ctr, RTBOOL rj) {   // (2.52.1) the centred flag honoured (rank text drawn centred)
    const int w = tw(t); const int l = ctr ? (left + right) / 2 - w / 2 : rj ? right - w : left;
    OPS.push_back(Op{"text", l, top, l + w, bottom, ink & 0xFFFFFF, 0, curFont, curBold, t, 0}); }
RTX_RESULT cppExtension::PNT::set(int bar, float price, BAR_POSITION) { h = (short)(X0 + PPB * bar); v = yOfP(price); return RTX_OK; }
void cppExtension::PNT::setDrawPosition() { posH = h; posV = v; }
void cppExtension::PNT::drawLineTo() { OPS.push_back(Op{"line", posH, posV, h, v, pen & 0xFFFFFF, 0, penW, false, "", penStyle}); posH = h; posV = v; }
void cppExtension::PNT::drawText(const char* s, int, COLOR, RTBOOL) { const int w = tw(s); OPS.push_back(Op{"text", h, v - curFont, h + w, v, ink & 0xFFFFFF, 0, curFont, curBold, s, 0}); }

#include "tests_dp290/dp_mock_cols.h"   // (CR5200) the 2.11.0 SDK calls (chart options, dialog setters)
// (2110) setup()'s parameter calls: the rows are recorded so the test knows each control's index (PX.*)
static std::vector<std::string> PARAMS;
void cppExtension::setParameterDialogHeight(int, int) {}
RTX_RESULT cppExtension::setParameterVersion(unsigned) { return RTX_OK; }
RTX_RESULT cppExtension::setBoolParameter(const char* n, RTBOOL b, short) { MOCK_CHK[PARAMS.size()] = b != 0; PARAMS.push_back(n); return RTX_OK; }
RTX_RESULT cppExtension::setIntegerParameter(const char* n, int v, short, short) { MOCK_INT[PARAMS.size()] = v; PARAMS.push_back(n); return RTX_OK; }
RTX_RESULT cppExtension::setListParameter(const char* n, short d, const char*, short, short) { MOCK_LIST[PARAMS.size()] = d; PARAMS.push_back(n); return RTX_OK; }
RTX_RESULT cppExtension::setStringParameter(const char* n, const char* d, size_t, short, short) { MOCK_TXT[PARAMS.size()] = d ? d : ""; PARAMS.push_back(n); return RTX_OK; }
// (2.20.0) the queue meter rows: a float (Reach) and three colours - mocked so setup() registers every row (a missing mock = a NULL call)
static double MOCK_FLT[32]; static unsigned long MOCK_COL[32];
RTX_RESULT cppExtension::setFloatParameter(const char* n, float f, short, short) { MOCK_FLT[PARAMS.size() % 32] = f; PARAMS.push_back(n); return RTX_OK; }
RTX_RESULT cppExtension::setColorParameter(const char* n, COLOR c, short, short) { MOCK_COL[PARAMS.size() % 32] = c; PARAMS.push_back(n); return RTX_OK; }
float cppExtension::getFloatValue(int i) { return i >= 0 && i < 32 ? (float)MOCK_FLT[i] : 0.0f; }
void cppExtension::setFloatValue(int i, float f) { if (i >= 0 && i < 32) MOCK_FLT[i] = f; }
COLOR cppExtension::getParameterColor(int i) { return i >= 0 && i < 32 ? (COLOR)MOCK_COL[i] : 0; }
RTX_RESULT cppExtension::setParameterColor(int i, COLOR c) { if (i >= 0 && i < 32) MOCK_COL[i] = c; return RTX_OK; }
int cppExtension::getParameterCount(void) { return (int)PARAMS.size(); }   // (2.17.1) SettingsHost.h registers by the count
RTX_RESULT cppExtension::enableParameter(int, RTBOOL) { return RTX_OK; }
static std::string UP;
static std::string fl(const char* name) { return UP + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void put(const char* name, const std::string& body) { std::ofstream f(fl(name).c_str(), std::ios::binary); f << body; f.close();
    // (2.17.1) the plugin reads DealerProfile.settings-<MKT>.txt (keyed) and migrates the old file once: a test that writes the OLD file
    // starts from it again (as a first start after the upgrade)
    if (std::string(name) == "DealerProfile.settings.txt") for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }) std::remove(fl((std::string("DealerProfile.settings-") + m + ".txt").c_str()).c_str()); }
static std::string keyedES() { return slurp(fl("DealerProfile.settings-ES.txt")); }   // (2.17.1) the market's keyed settings file
static const std::string D = "tests_dp290";

static Chart loadBars(const char* cut, const std::string& root = "ES", const std::string& sym = "ESZ6", float shift = 0)
{
    Chart a; a.root = root; a.sym = sym;
    std::istringstream in(slurp(D + "/LRA-IRT-Bars-ES.csv")); std::string ln; std::getline(in, ln);
    while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() < 6) continue;
        if (c[0].substr(11, 5) > std::string(cut)) break;
        struct tm t; memset(&t, 0, sizeof(t)); strptime(c[0].c_str(), "%Y-%m-%d %H:%M:%S", &t); time_t u = timegm(&t);
        a.dt.push_back((unsigned long)u); a.o.push_back(std::stof(c[1]) + shift); a.h.push_back(std::stof(c[2]) + shift); a.l.push_back(std::stof(c[3]) + shift); a.c.push_back(std::stof(c[4]) + shift); a.v.push_back(std::stoul(c[5])); }
    return a;
}
static void prime(DealerProfile& X, Chart& a, const std::string& mkt, const std::vector<klv::Merged>* levels = nullptr)
{
    CUR = &a; X.loadHostState();
    DealerProfile::KLC K; K.n = (long)a.dt.size(); K.lastT = (long long)a.dt.back(); K.at = time(nullptr);
    if (levels) K.v = *levels;
    else { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
        while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") K.v.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    X.klc[mkt] = K; X.saveHostState();
}
static std::vector<Op> drawOn(DealerProfile& X, Chart& a, const std::string& label)
{
    CUR = &a; MOCK_CHART = label; NOW = a.dt.back() + 180; OPS.clear(); X.draw(); return OPS;
}
static DealerProfile::ColsState colsOf(DealerProfile& X, Chart& a) { CUR = &a; X.loadHostState(); DealerProfile::ColsState c = X.cs; return c; }
static bool inBox(const Op& q, const bp::Box& b) { return q.l >= b.l && q.r <= b.r; }
static bool hasText(const std::vector<Op>& O, const std::string& s) { for (auto& q : O) if (q.kind == "text" && q.text == s) return true; return false; }
static int countPrefix(const std::vector<Op>& O, const std::string& s) { int n = 0; for (auto& q : O) if (q.kind == "text" && q.text.rfind(s, 0) == 0) ++n; return n; }
static void dealerFile(bool fresh)
{
    std::string d = slurp(D + "/LRA-Dealer-ES.csv");
    if (fresh) { size_t p = d.find("BOOKASOF|17700"); if (p != std::string::npos) d.replace(p, 14, "BOOKASOF|53700"); }
    put("LRA-Dealer-ES.csv", d);
}
static void settingsRow(const std::string& key, int dealerW, int levelsW, int liq, int font = 10)
{
    std::map<std::string, std::string> rows;
    { std::istringstream in(slurp(fl("DealerProfile.settings.txt"))); std::string ln; while (std::getline(in, ln)) { size_t p = ln.find('|'); if (p != std::string::npos) rows[ln.substr(0, p)] = ln; } }
    rows[key] = key + "|0|" + std::to_string(dealerW) + "|" + std::to_string(font) + "|0|1|1|1|0|cob_bull,cob_bear|c2|" + std::to_string(levelsW) + "|9|" + std::to_string(liq);
    std::string body; for (auto& kv : rows) body += kv.second + "\n";
    put("DealerProfile.settings.txt", body);
}
// every Dealer Profile op right of the price area must sit in the Levels or the Dealer column (level LINES run across: allowed),
// nothing in the Delta column; returns the count of offenders
static int strays(const std::vector<Op>& O, const DealerProfile::ColsState& c, std::string* first = nullptr)
{
    int n = 0;
    for (const Op& q : O) {
        if (q.kind == "line") continue;
        if (q.r <= c.lay.priceRight) continue;
        if (q.l >= c.lay.priceRight && (inBox(q, c.lay.levels) || inBox(q, c.lay.dealer))) continue;
        ++n; if (first && first->empty()) *first = q.kind + " " + q.text + " " + std::to_string(q.l) + "-" + std::to_string(q.r);
    }
    return n;
}
static int badgeOverlaps(const std::vector<Op>& O, const bp::Box& col, int* count = nullptr)
{
    std::vector<Op> b; for (const Op& q : O) if (q.kind == "box" && q.l == col.l + 2 && q.r <= col.r) b.push_back(q);   // the name blocks (2 px from the column edge)
    if (count) *count = (int)b.size();
    int ov = 0; for (size_t i = 0; i < b.size(); ++i) for (size_t j = i + 1; j < b.size(); ++j) if (b[i].t < b[j].b && b[j].t < b[i].b) ++ov;
    return ov;
}
static void liqFile(const std::string& rows, long long ageS = 0)
{
    put("LRA-LiqLevels-ES.csv", "LIQL,1," + std::to_string((long long)time(nullptr) - ageS) + ",0.25,2.5,2.1,4\n" + rows);
}
static void reloadLiq(DealerProfile& X, Chart& a) { CUR = &a; X.loadHostState(); X.cs.liqLChecked = -1; X.cs.liqLStamp = -2; X.saveHostState(); }

static const char* DUMP = nullptr;   // argv[1]: the liquidity scene's draw calls for the render check (render_cols.py)
static void dump(const std::vector<Op>& R, const Chart& a, const DealerProfile::ColsState& c)
{
    FILE* f = std::fopen(DUMP, "w"); if (!f) return;
    std::fprintf(f, "pane|0|0|%d|%d|%d\n", PW, PH, PW + 60);
    std::fprintf(f, "cols|%d|%d|%d|%d|%d|%d|%d\n", c.lay.priceRight, c.lay.levels.l, c.lay.levels.r, c.lay.delta.l, c.lay.delta.r, c.lay.dealer.l, c.lay.dealer.r);
    for (size_t i = 0; i < a.dt.size(); i++) std::fprintf(f, "bar|%d|%d|%d|%d|%d\n", X0 + PPB * (int)i, yOfP(a.o[i]), yOfP(a.h[i]), yOfP(a.l[i]), yOfP(a.c[i]));
    for (int p = 7800; p <= 7930; p += 10) std::fprintf(f, "tick|%d|%d\n", yOfP((float)p), p);
    for (const Op& q : R) {
        if (q.kind == "text") std::fprintf(f, "text|%d|%d|%d|%d|%06lx|%d|%d|%s\n", q.l, q.t, q.r, q.b, q.col, q.font, q.bold ? 1 : 0, q.text.c_str());
        else if (q.kind == "line") std::fprintf(f, "line|%d|%d|%d|%d|%06lx|%d\n", q.l, q.t, q.r, q.b, q.col, q.font);
        else std::fprintf(f, "%s|%d|%d|%d|%d|%06lx|%06lx|%d\n", q.kind.c_str(), q.l, q.t, q.r, q.b, q.col, q.col2, q.font);
    }
    std::fclose(f);
}
static std::string badgeFile(const std::string& mkt, int hod1, int lod1)
{
    std::string s = "VERSION,1\nMARKET," + mkt + "\nBUILT,2026-10-21\nZONE_ADR,0.25\nDAYS,2025-05-27,2026-10-20\n# L,name,side,n1,pct1,rnd1,n2,pct2,rnd2,beats\n";
    for (const char* n : { "PWL", "PFL", "PDL", "PDH", "PFH", "PWH", "CR0", "HVL", "PS0", "CR", "PS" }) {
        s += std::string("L,") + n + ",HOD,150," + std::to_string(hod1) + ",9,320,40,30," + (std::string(n) == "PWH" ? "3" : "0") + "\n";
        s += std::string("L,") + n + ",LOD,150," + std::to_string(lod1) + ",9,320,41,30," + (std::string(n) == "PDL" ? "1" : "0") + "\n";
    }
    return s;
}
static bool isPct(const std::string& t) { return t.find("%  /  ") != std::string::npos; }
int main(int argc, char** argv)
{
    if (argc > 1) DUMP = argv[1];
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2560XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp");
        check(std::string(DP_VERSION) == "2.61.0" && src.find("p->setVersion(\"2.61.0\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && slurp("LimitProfile.cpp").find("p->setVersion(\"2.61.0\")") != std::string::npos, "versions 2.61.0 agree (DealerProfile, settings, LimitProfile)"); }
    dpGHoldSec() = 0;   // the 2.56.0 scenes below repost within one minute
    dealerFile(true);
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 150);
    drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 150);
    // ---- A) no kind G: the old path, not native
    {   DealerProfile::ColsState c = colsOf(X, a); check(!c.gNative && c.gShown == 0, "no kind G = not native"); }
    // ---- B) kind G: 3 bids below 7853.25, 3 offers above, ranks 1-6 as posted (K = 900 offer, mostly one order; one bid S20)
    struct N { double px; long long lots; int rank; int orders; int tag; };   // tag proven bits: 2 = "?", 4 = S20
    const N g[] = { { 7858.00, 900, 1, 20, 2 + 8 * 49 }, { 7845.00, 500, 2, 28, 0 + 8 * 93 }, { 7870.00, 490, 3, 34, 0 }, { 7820.00, 480, 4, 21, 4 }, { 7900.00, 470, 5, 11, 0 }, { 7841.00, 455, 6, 22, 6 } };
    std::vector<sb::Fact> G;
    for (const N& n : g) { sb::Fact f; f.bar = n.orders; f.lo = f.hi = n.px; f.side = n.px < 7853.25 ? 1 : -1; f.val = (double)n.lots; f.code = n.rank; f.proven = n.tag; G.push_back(f); }
    { sb::Fact h; h.code = 99; h.val = 10318; h.state = 1; G.push_back(h); }
    sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), G);
    std::vector<sb::Fact> B; { sb::Fact f; f.bar = 1; f.lo = f.hi = 7850.0; f.side = 1; f.val = 5000; f.code = 4; B.push_back(f); } sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B);   // an old LP feed too: G wins
    std::vector<Op> O = drawOn(X, a, MOCK_CHART);
    DealerProfile::ColsState c = colsOf(X, a);
    if (DUMP) dump(O, a, c);
    check(c.gNative && c.gShown == 6 && c.gBook == 10318 && c.gSingle == 1 && c.gS20 == 2 && c.gReal == 2, "native: 6 shown, book 10318, 1 '?', 2 S20 (" + std::to_string(c.gShown) + " / " + std::to_string(c.gSingle) + " / " + std::to_string(c.gS20) + ")");
    check(c.bookMode && c.qFromBoard && !c.rkHide, "drawn through the book rows (rank | bar | lots)");
    const int lqL = c.lay.levels.l + c.keysW, lqR = c.lay.levels.r;
    std::map<std::string, int> T; for (auto& q : O) if (q.kind == "text" && q.l >= lqL - 2 && q.r <= lqR + 2) T[q.text]++;
    {   std::string all; for (auto& kv : T) all += "[" + kv.first + "]";
        check(T["900  100% ?"] == 1 && T["500  56%"] == 1 && T["490  54%"] == 1 && T["480  53% S20"] == 1 && T["470  52%"] == 1 && T["455  51% S20"] == 1, "node texts: lots  pct%  [tag] " + all);
        check(c.gNodes == 6, "6 nodes drawn (" + std::to_string(c.gNodes) + ")");
        int inside = 0, anch = 0, rkOut = 0, rkBox = 0, n = 0;
        for (auto& q : O) { if (q.kind != "text" || q.l < lqL - 2 || q.r > lqR + 2) continue; const bool node = q.text.find('%') != std::string::npos; if (!node) continue; ++n;
            const Op* nb_ = nullptr; for (auto& z : O) if (z.kind == "box" && z.r == lqR - 3 && z.t < q.b && z.b > q.t + 4 && z.l >= lqL && z.col != 0x00FFFFFF) { if (!nb_ || z.l < nb_->l) nb_ = &z; }
            if (nb_) { ++anch; if (q.r <= lqR - 2) ++inside; }
            for (auto& r : O) if (r.kind == "text" && (r.text == "K" || (r.text.size() == 1 && r.text[0] >= '2' && r.text[0] <= '6')) && std::abs((r.t + r.b) / 2 - (q.t + q.b) / 2) <= 2 && r.l >= lqL - 2) { if (r.r <= q.l && (!nb_ || r.r <= nb_->l)) ++rkOut;
                for (auto& z : O) if (z.kind == "box" && z.font == 1 && z.l <= r.l && z.r >= r.r && z.t < r.b && z.b > r.t + 4 && z.r - z.l < 40 && z.l >= lqL) ++rkBox; } }
        check(n == 6 && anch == 6 && inside == 6, "every node is anchored at the outer edge and its text ends inside it (" + std::to_string(anch) + " / " + std::to_string(inside) + ")");
        check(rkOut == 6 && rkBox == 0, "the rank is outside, in front of the node, no box (" + std::to_string(rkOut) + " / boxes " + std::to_string(rkBox) + ")");
        std::printf("badges %d  texts %s\n", c.gBadges, all.c_str()); }
    check(T["K"] == 1 && T["2"] == 1 && T["3"] == 1 && T["4"] == 1 && T["5"] == 1 && T["6"] == 1, "rank boxes K, 2-6 as posted");
    check(T.count("5000") == 0, "the old lsLiquidityProfile node is not shown when kind G is there");
    {   int ascii = 1; for (auto& q : O) if (q.kind == "text" && q.l >= lqL - 2 && q.r <= lqR + 2) for (unsigned char ch : q.text) if (ch > 126) ascii = 0; check(ascii == 1, "plain ASCII in the Liquidity rows"); }
    {   const std::string st = slurp(fl("DealerProfile.status-ES-180.txt")); check(st.find("TOP_BOOK_G,NATIVE") != std::string::npos, "status TOP_BOOK_G NATIVE"); }
    // ---- C) heartbeat only (book 0 = cold book after a reconnect): native, nothing ranked
    {   std::vector<sb::Fact> H; sb::Fact h; h.code = 99; h.val = 0; H.push_back(h); sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), H);
        std::vector<Op> O2 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c2 = colsOf(X, a);
        int k = 0; for (auto& q : O2) if (q.kind == "text" && q.text == "K") ++k;
        check(c2.gNative && c2.gShown == 0 && c2.rkHide && k == 0, "cold book: native, no rows, no K"); }
    // ---- D) (2.56.0) an ES Pdoc long from lsBreakout (fact kind E): "ENT Pdoc" on the PB POC row of the Key Levels column
    {   std::vector<sb::Fact> G2; for (const N& n : g) { sb::Fact f; f.bar = n.orders; f.lo = f.hi = n.px; f.side = n.px < 7853.25 ? 1 : -1; f.val = (double)n.lots; f.code = n.rank; f.proven = n.tag; G2.push_back(f); }
        { sb::Fact h; h.code = 99; h.val = 10318; h.state = 1; G2.push_back(h); } sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), G2);
        const size_t n = a.dt.size(); const size_t k = n - 4;   // the decision bar: 2 closed bars after it (the last bar forms)
        float lo = 1e9f, hi = -1e9f; for (size_t j = k - 10; j < n; ++j) { lo = std::min(lo, a.l[j]); hi = std::max(hi, a.h[j]); }
        const double atr = X.atr14(); const double pbPoc = std::floor((a.c[k] - std::max(0.25, 0.4 * atr)) * 4.0) / 4.0;
        sb::Fact e; e.bar = (long long)a.dt[k]; e.side = 1; e.code = 3; e.lo = std::min(pbPoc, (double)lo + 1.0); e.hi = (double)lo - 2.0; e.val = (double)hi + 20.0; e.state = 1;
        std::printf("E fact: close %.2f atr %.2f pbPoc %.2f stop %.2f target %.2f\n", a.c[k], atr, e.lo, e.hi, e.val);
        sb::postFacts('E', a.sym, a.spb, MOCK_CHART, dpNowMs(), std::vector<sb::Fact>{ e });
        std::vector<Op> O3 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c3 = colsOf(X, a);
        const int kL = c3.lay.levels.l, kR = c3.lay.levels.l + c3.keysW;
        int inKeys = 0, elsewhere = 0; for (auto& q : O3) if (q.kind == "text" && q.text.find("ENT") != std::string::npos) { if (q.l >= kL - 2 && q.r <= kR + 2) ++inKeys; else ++elsewhere; }
        std::printf("ENTRY: why %s tags %zu drawn %d added %d inKeys %d elsewhere %d\n", c3.eb.why.c_str(), c3.eb.tags.size(), c3.ebDrawn, c3.ebAdded, inKeys, elsewhere);
        check(c3.eb.why == "ok" && c3.eb.tags.size() == 1 && c3.eb.tags[0].text == "ENT Pdoc", "ES Pdoc -> one ENT Pdoc tag");
        check(inKeys == 1 && elsewhere == 0 && c3.ebDrawn == 1, "ENT Pdoc drawn once, inside the Key Levels column (no new column)");
        check(hasText(O3, "PbPOC"), "the PB POC row added to the Key Levels column while the tag lives");
        {   const std::string st = slurp(fl("DealerProfile.status-ES-180.txt")); check(st.find("ENTRY_BADGE,EntryBadge 1.1.0,E facts 1,label Pdoc,state ok") != std::string::npos, "status ENTRY_BADGE");
            check(st.find("params no file - defaults (ENT_PDOC ON ENT_GAMMA OFF EXIT_BOOK OFF SCALE OFF)") != std::string::npos, "(2.57.0) no params file -> the compiled ES defaults, in the status line"); }
        e.code = 1; sb::postFacts('E', a.sym, a.spb, MOCK_CHART, dpNowMs(), std::vector<sb::Fact>{ e });
        std::vector<Op> O4 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c4 = colsOf(X, a);
        int ent = 0; for (auto& q : O4) if (q.kind == "text" && q.text.find("ENT") != std::string::npos) ++ent;
        check(ent == 0 && c4.eb.tags.empty() && !hasText(O4, "PbPOC"), "a Pb (not Pdoc) -> no ENT tag, no PB POC row");
        sb::postFacts('E', a.sym, a.spb, MOCK_CHART, dpNowMs() - 120000, std::vector<sb::Fact>{ e });
        std::vector<Op> O5 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c5 = colsOf(X, a);
        check(c5.ebFacts == -1 && c5.eb.tags.empty(), "stale / no E producer -> no tag");
        // (2.57.0, B-20261109-LQ1) the nightly params LRA-EntryBadge-ES.csv. 1) the SAME label keeps its decided tag when the file changes
        // (no repaint); 2) a NEW label uses the file: OFF -> no ENT, "?" -> "ENT Pdoc ?", garbled -> the compiled default "ENT Pdoc"
        auto writeParams = [&](const std::string& body) { char ts[32]; std::time_t t0 = std::time(nullptr); struct tm lt; localtime_r(&t0, &lt); std::strftime(ts, sizeof ts, "%Y-%m-%dT%H:%M:%S", &lt);
            std::ofstream f(fl("LRA-EntryBadge-ES.csv"), std::ios::binary); f << "# LRA-EntryBadge-ES.csv - liq_learn 1.0.0 " << ts << " - test\n" << body; };
        e.code = 3; sb::postFacts('E', a.sym, a.spb, MOCK_CHART, dpNowMs(), std::vector<sb::Fact>{ e });
        { std::vector<Op> Oa = drawOn(X, a, MOCK_CHART); writeParams("status,ENT_PDOC,OFF\n"); dpEbClockSkew += 61;
          std::vector<Op> Ob = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState cb2 = colsOf(X, a);
          check(cb2.ebPar.why == "file" && cb2.eb.tags.size() == 1 && cb2.eb.tags[0].text == "ENT Pdoc", "(2.57.0) the same label keeps its tag when the nightly turns it OFF (no repaint)"); }
        const char* BODY[3] = { "status,ENT_PDOC,OFF\n", "status,ENT_PDOC,?\n", "status,ENT_PDOC,banana\nnot a line\n" };
        const char* WANT[3] = { "", "ENT Pdoc ?", "ENT Pdoc" };
        for (int z = 0; z < 3; ++z) { writeParams(BODY[z]); dpEbClockSkew += 61;
            sb::Fact e2 = e; e2.bar = e.bar - 180LL * (z + 1);   // a NEW (older-bar) label id -> a new snapshot
            const size_t k2 = k - (size_t)(z + 1); e2.lo = std::floor((a.c[k2] - std::max(0.25, 0.4 * X.atr14())) * 4.0) / 4.0; e2.lo = std::min(e2.lo, (double)lo + 1.0);
            sb::postFacts('E', a.sym, a.spb, MOCK_CHART, dpNowMs(), std::vector<sb::Fact>{ e2 });
            std::vector<Op> O6 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c6 = colsOf(X, a);
            const std::string got = c6.eb.tags.empty() ? std::string() : c6.eb.tags[0].text;
            std::printf("PARAMS %d: why %s tags %zu text '%s' params %s\n", z, c6.eb.why.c_str(), c6.eb.tags.size(), got.c_str(), c6.ebPar.why.c_str());
            check(got == WANT[z], std::string("(2.57.0) new label with params case ") + std::to_string(z) + " -> '" + WANT[z] + "'"); }
        std::remove(fl("LRA-EntryBadge-ES.csv").c_str()); }
    {   // ---- (2.59.0) E) once a minute: a repost inside the minute is not drawn; a touch (price within 2 ticks of a node) or hold 0 shows it at once
        auto post = [&](long long kingLots, double kingPx) { std::vector<sb::Fact> Gx; for (const N& n : g) { sb::Fact f; f.bar = n.orders; f.lo = f.hi = n.rank == 1 ? kingPx : n.px; f.side = f.lo < 7853.25 ? 1 : -1; f.val = (double)(n.rank == 1 ? kingLots : n.lots); f.code = n.rank; f.proven = 0; Gx.push_back(f); }
            { sb::Fact h; h.code = 99; h.val = 10318; h.state = 1; Gx.push_back(h); } sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), Gx); };
        auto has = [&](const std::string& pre) { std::vector<Op> Ox = drawOn(X, a, MOCK_CHART); for (auto& q : Ox) if (q.kind == "text" && q.text.compare(0, pre.size(), pre) == 0) return true; return false; };
        dpGHoldSec() = 0; post(900, 7858.00); check(has("900  100%"), "hold 0: drawn at once");
        dpGHoldSec() = 100000000; post(900, 7858.00); (void)has("900");                       // one slot for the whole test
        post(950, 7858.00); check(has("900  100%") && !has("950  100%"), "inside the minute: the earlier nodes stay");
        { DealerProfile::ColsState c5 = colsOf(X, a); check(c5.gHeld >= 1, "status counts the held draws (" + std::to_string(c5.gHeld) + ")"); }
        const double last = a.c.back(); post(960, std::round(last * 4.0) / 4.0 + 0.25); (void)has("9");   // the held set has no node at price yet -> still held
        check(has("900  100%"), "a node that only the NEW post has at price does not break the hold");
        dpGHoldSec() = 0; post(960, std::round(last * 4.0) / 4.0 + 0.25); (void)has("960"); dpGHoldSec() = 100000000; post(970, std::round(last * 4.0) / 4.0 + 0.25);
        check(has("970  100%"), "price within 2 ticks of a held node: updated at once");
        {   std::vector<sb::Fact> H; sb::Fact h; h.code = 99; h.val = 0; H.push_back(h); sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), H); std::vector<Op> Oc = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c6 = colsOf(X, a);
            check(c6.gShown == 0 && c6.gNodes == 0, "cold book inside the minute: cleared at once"); }
        // ---- F) the level badge: a key level within 1 ATR names the LARGEST node only; farther than 1 ATR = no badge
        dpGHoldSec() = 0; const double atr = X.atr14(); double kp = 0; std::string kn;
        for (const klv::Merged& m : lv) if (m.price > last + 6 * atr && (kp == 0 || m.price < kp)) { kp = m.price; kn = m.name; }
        if (kp > 0) {
            std::vector<sb::Fact> Gx; auto nd = [&](double px, long long lots, int rank) { sb::Fact f; f.bar = 20; f.lo = f.hi = px; f.side = px < last ? 1 : -1; f.val = (double)lots; f.code = rank; Gx.push_back(f); };
            const double q4 = 0.25; auto r4 = [&](double p) { return std::round(p / q4) * q4; };
            nd(r4(kp + 0.5 * atr), 800, 1); nd(r4(kp - 0.5 * atr), 300, 2); nd(r4(kp + 3.0 * atr), 250, 3); { sb::Fact h; h.code = 99; h.val = 10318; h.state = 1; Gx.push_back(h); }
            sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), Gx); std::vector<Op> Ob = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c7 = colsOf(X, a);
            int bx = 0, onKing = 0; const int lqL2 = c7.lay.levels.l + c7.keysW;
            for (auto& q : Ob) if (q.kind == "text" && q.l >= lqL2 - 2 && q.col == ((unsigned long)C_DARK & 0xFFFFFF) && q.bold) { for (auto& z : Ob) if (z.kind == "box" && z.col == 0x00FFFFFF && z.l <= q.l && z.r >= q.r && z.t < q.b && z.b > q.t + 4) { ++bx;
                for (auto& m : Ob) if (m.kind == "text" && m.text.compare(0, 9, "800  100%") == 0 && std::abs((m.t + m.b) / 2 - (q.t + q.b) / 2) <= 2 && q.r <= m.l) ++onKing; break; } }
            check(c7.gBadges >= 1 && bx >= 1 && onKing == 1, "level " + kn + " within 1 ATR: a white badge left of the lots on the LARGEST node (" + std::to_string(c7.gBadges) + " / " + std::to_string(bx) + " / " + std::to_string(onKing) + ")");
            {   // (2.60.0) the level's line is bright (2 px, red = offers) on the last candle, inside the price area, and carries no number
                int br = 0, num = 0; const int yk = yOfP((float)kp);
                for (auto& q : Ob) if (q.kind == "line" && q.font == 2 && q.col == ((unsigned long)C_SELL & 0xFFFFFF) && std::abs(q.t - yk) <= 1 && q.t == q.b && q.r <= c7.lay.levels.l && q.r > q.l) ++br;
                for (auto& q : Ob) if (q.kind == "text" && q.r <= c7.lay.levels.l && std::abs((q.t + q.b) / 2 - yk) <= 8 && q.text.find("800") != std::string::npos) ++num;
                check(c7.gSegs.size() >= 1 && c7.gSegDrawn >= 1 && br >= 1 && num == 0, "the level line is bright while its node is there, no amount on the chart (" + std::to_string(c7.gSegs.size()) + " / " + std::to_string(c7.gSegDrawn) + " / " + std::to_string(br) + ")");
                { sb::Fact h; h.code = 99; h.val = 10318; h.state = 1; std::vector<sb::Fact> G0; nd(r4(kp + 3.0 * atr), 250, 1); G0.push_back(Gx.back()); G0.push_back(h); sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), G0); }
                std::vector<Op> Oc2 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c8 = colsOf(X, a);
                {   int files = 0, lines = 0, good = 0; for (auto& e : std::filesystem::directory_iterator(std::filesystem::path(UP).parent_path())) { const std::string fn = e.path().filename().string(); if (fn.find("ES-liqlines-") == std::string::npos) continue; ++files;
                        std::istringstream in(slurp(e.path().string())); std::string ln; while (std::getline(in, ln)) { if (ln.compare(0, 2, "L|") != 0) continue; ++lines; std::vector<std::string> f = dl::split(ln, '|'); if (f.size() == 9 && f[3] == kn && f[4] == "-1" && std::atoll(f[6].c_str()) >= std::atoll(f[5].c_str()) && std::atoll(f[7].c_str()) > 0) ++good;   /* the stretch began in scene B with the 470 node: one line a minute */ } }
                    check(files == 1 && good >= 1 && c7.gSegLines >= 1 && c7.gSegFails == 0, "the stretch is recorded: " + std::to_string(files) + " file, " + std::to_string(lines) + " lines, " + std::to_string(good) + " for " + kn);
                    const int before = lines; drawOn(X, a, MOCK_CHART); drawOn(X, a, MOCK_CHART); int after = 0; for (auto& e : std::filesystem::directory_iterator(std::filesystem::path(UP).parent_path())) { if (e.path().filename().string().find("ES-liqlines-") == std::string::npos) continue; std::istringstream in(slurp(e.path().string())); std::string ln; while (std::getline(in, ln)) if (ln.compare(0, 2, "L|") == 0) ++after; }
                    check(after == before, "no new line on a repaint inside the minute (" + std::to_string(before) + " -> " + std::to_string(after) + ")"); }
                check(c8.gBadges == 0 && c8.gSegs.size() == c7.gSegs.size() && c8.gSegDrawn >= 1, "the node left: the stretch stays where it was (kept " + std::to_string(c8.gSegs.size()) + ", drawn " + std::to_string(c8.gSegDrawn) + ")");
                sb::postFacts('G', a.sym, a.spb, MOCK_CHART, dpNowMs(), Gx); Ob = drawOn(X, a, MOCK_CHART); }
            int two = 0; for (auto& q : Ob) if (q.kind == "text" && q.text.compare(0, 4, "300 ") == 0) for (auto& z : Ob) if (z.kind == "box" && z.col == 0x00FFFFFF && z.r <= q.l && z.r >= q.l - 8 && std::abs((z.t + z.b) / 2 - (q.t + q.b) / 2) <= 2) ++two;
            check(two == 0, "the smaller node at the same level gets no badge");
        } else check(false, "no key level above price in the fixture");
        dpGHoldSec() = 60; }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
