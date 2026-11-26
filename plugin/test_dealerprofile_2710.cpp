// test_dealerprofile_2710.cpp -- 2.71.0 (2026-11-26, Rassul 2026-11-25 "hide the limit side on the right and keep the levels on the left displayed"): the LIQUIDITY
// column is not drawn; the published Levels width is the Key Levels column alone; the title "LP v2.71.0" sits at the top of the Key Levels column.
// The harness below is test_dealerprofile_2691.cpp's, unchanged; only main() is new.
// The harness below is test_dealerprofile_2680.cpp's, unchanged (real irtsdk.h, the SDK calls mocked in this file); only main() is new.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -w -DDP_LIMIT_NAME=1 -isystem <sdk include> -I. test_dealerprofile_2691.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t [ops-dump-file]
// test_dealerprofile_2680.cpp -- 2.68.0 (2026-11-20): LevelLiq.h on the level lines (logic in test_levelliq_100.cpp); this host suite = 2670's with the version + a LEVEL LIQ status check.
// test_dealerprofile_2670.cpp -- 2.67.0 (2026-11-20, Rassul 05:05 CT): ONE KING + its L / P marks (KingMark.h 1.0.0, TopBook 1.3.0, LimitFeed 1.2.0). Built on test_dealerprofile_2651.cpp:
// its scenes kept, the column expectations moved to one king (no ranks 2-6, no % on the king row); new scene K = the king / mark logic on the real feed code.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -Werror -Wall -Wextra -DDP_LIMIT_NAME=1 -I<sdk include> -I. test_dealerprofile_2670.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
#include <chrono>
// test_dealerprofile_2651.cpp -- 2.65.1: PERFORMANCE FIX - the status files are no longer replaced on every draw (WriteGate min gap 5 s, XLINES counter per draw). Built on test_dealerprofile_2630.cpp (its 44 checks kept, the gap set to 0 for them).
// test_dealerprofile_2630.cpp -- 2.63.0: THE OWN ORDER FEED (LimitFeed.h): the top book without lsBookFlow. Built on test_dealerprofile_2620.cpp (its scenes run with the own feed off).
// test_dealerprofile_2620.cpp -- 2.62.0: Limit Profile only (compile with -DDP_LIMIT_NAME=1: header "LP v2.63.0"). Built on test_dealerprofile_2610.cpp.
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
static long long GLT_CALLS = 0, PNT_SETS = 0;   // (2640) the SDK calls a draw makes
struct tm* cppExtension::getLocaltime(RTDATE d, struct tm* p) { ++GLT_CALLS; time_t t = (time_t)d; gmtime_r(&t, p); return p; }
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
RTX_RESULT cppExtension::PNT::set(int bar, float price, BAR_POSITION) { ++PNT_SETS; h = (short)(X0 + PPB * bar); v = yOfP(price); return RTX_OK; }
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
// (2.63.0) the mocked order feed: one queue; the constructor subscribes, the destructor is counted
static std::vector<DEPTH_ORDER> MBOQ; static size_t MBOI = 0; static int MBO_OPEN = 0, MBO_CLOSE = 0; static long long TCLK = 0;
cppExtension::DEPTH_BY_ORDER::DEPTH_BY_ORDER(const char* t) { index = 0; bDboSubscribed = 1; std::memset(symbol, 0, sizeof symbol); if (t) std::strncpy(symbol, t, sizeof symbol - 1); std::memset(&dbo, 0, sizeof dbo); ++MBO_OPEN; }
cppExtension::DEPTH_BY_ORDER::~DEPTH_BY_ORDER() { ++MBO_CLOSE; }
RTX_RESULT cppExtension::DEPTH_BY_ORDER::getNext(void) { if (MBOI >= MBOQ.size()) return RTX_FAIL; dbo = MBOQ[MBOI++]; return RTX_OK; }
static void mbo(int action, bool bid, double px, unsigned long sz, int oid) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.action = (unsigned char)action; o.buySell = bid ? 1 : 0; o.price = (float)px; o.size = sz; o.orderID = oid; MBOQ.push_back(o); }
// ---- 2.69.1: the R / S colour of a level line ----
static const Op* longLineAt(const std::vector<Op>& O, int y, int keysL)
{
    for (const Op& q : O) if (q.kind == "line" && q.t == y && q.b == y && q.l <= 2 && q.r == keysL - 1) return &q;   // from the pane's left edge to the Key Levels column
    return nullptr;
}
static const Op* textOp(const std::vector<Op>& O, const std::string& s) { for (const Op& q : O) if (q.kind == "text" && q.text == s) return &q; return nullptr; }
// the filled badge box under a name: the smallest box that holds the text's left end
static const Op* badgeOf(const std::vector<Op>& O, const Op& t)
{
    const Op* best = nullptr;
    for (const Op& z : O) if (z.kind == "box" && z.l <= t.l && z.r >= t.r && z.t <= t.t + 8 && z.b >= t.b - 2 && z.col != 0x000B0F19) { if (!best || z.r - z.l < best->r - best->l) best = &z; }
    return best;
}
static unsigned long c24(COLOR c) { return (unsigned long)c & 0xFFFFFF; }
int main(int argc, char** argv)
{
    (void)argc; (void)argv; (void)&dump; (void)&badgeFile; (void)&isPct; (void)&inBox; (void)&hasText; (void)&countPrefix; (void)&keyedES; (void)&badgeOf; (void)&c24; (void)&longLineAt; (void)&textOp;
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2710XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp"), lp = slurp("LimitProfile.cpp"); const std::string sv = std::string("p->setVersion(\"") + DP_VERSION + "\")";
        check(src.find(sv) != std::string::npos && lp.find(sv) != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && std::string(DP_VERSION) == "2.71.0", "versions agree: 2.71.0");
        check(!DP_LIQ_COL, "DP_LIQ_COL is off"); }
    dpGHoldSec() = 0; dpaudit::statusMinGapSec() = 0; dealerFile(true);
    for (int feed = 0; feed < 2; ++feed) {
        dpOwnFeed() = feed == 1;
        if (feed == 1) { MBOQ.clear(); MBOI = 0; for (int i = 0; i < 6; ++i) { mbo(1, true, 7830.0 - i * 0.25, 400 + 50 * i, 9000 + i); mbo(1, false, 7860.0 + i * 0.25, 380 + 40 * i, 9100 + i); } }
        std::vector<klv::Merged> lv;
        lv.push_back(klv::Merged{ "PWL", 7760.25, false }); lv.push_back(klv::Merged{ "PFL/PDL/S1", 7834.5, false }); lv.push_back(klv::Merged{ "PDH/R2", 7863.25, false });
        lv.push_back(klv::Merged{ "PFH", 7880.0, false }); lv.push_back(klv::Merged{ "PWH", 7897.5, false });
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
        MOCK_CHART = feed ? "ES 3m #F" : "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 150);
        std::vector<Op> O = drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 150);
        O = drawOn(X, a, MOCK_CHART);
        const DealerProfile::ColsState c = colsOf(X, a); const int kL = c.lay.levels.l, kR = c.lay.levels.r;
        const std::string tag = feed ? " [own feed]" : " [no feed]";
        check(c.lay.levels.w() > 0 && c.lay.levels.w() <= DP_KEYS_RW + 2, "the published Levels width is the Key Levels column alone (" + std::to_string(c.lay.levels.w()) + ")" + tag);
        check(c.keysW == c.lay.levels.w(), "the Key Levels column takes the whole width (" + std::to_string(c.keysW) + ")" + tag);
        bool liqText = false, ground = false, outside = false; const Op* head = nullptr; int badges = 0, lines = 0;
        for (const Op& q : O) {
            if (q.kind == "text" && q.text.find("LIQUIDITY") != std::string::npos) liqText = true;
            if (q.kind == "text" && q.text.rfind("LP v2.71.0", 0) == 0) head = &q;
            if (q.kind == "box" && q.col == 0x000B0F19UL && q.b - q.t > 100) ground = true;   // the full-height column ground (badge grounds are one row)
            if (q.kind != "line" && q.l > kR + 1) outside = true;
            if (q.kind == "text" && (q.text == "PDH/R2" || q.text == "PFL/PDL/S1" || q.text == "PFH")) ++badges;
            if (q.kind == "line" && q.t == q.b && q.l <= 2) ++lines; }
        check(!liqText, "no LIQUIDITY header / warming text" + tag);
        check(!ground, "no Liquidity column ground" + tag);
        check(!outside, "nothing drawn right of the Key Levels column" + tag);
        check(head && head->l >= kL && head->l <= kL + 12 && head->t < 40, "the title 'LP v2.71.0' at the top of the Key Levels column" + tag);
        check(badges >= 3, "the key level badges are drawn (" + std::to_string(badges) + ")" + tag);
        check(lines >= 4 && lines == c.xlines, "the level lines across the chart unchanged (" + std::to_string(lines) + ")" + tag);
        check(!X.muteDraw, "the mute flag is off after the draw" + tag);
        CUR = &a; X.destroy();
    }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
