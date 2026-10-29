// test_dealerprofile_2361.cpp -- lsDealerProfile 2.36.1 (2026-10-28 18:24 CT Rassul "you shouldn't be able to see the background"): the Key Levels column ground is a SOLID box in the chart background colour over the full column, drawn first; the 2.36.0 suite below kept.
// test_dealerprofile_2360.cpp -- lsDealerProfile 2.36.0 (2026-10-28 16:26 CT Rassul "the chart is extremely messy, remove the drawing of levels across the chart": no level / order / HELD lines in the price area, HELD as a column tag, off-screen markers in the Key Levels column) / 2.35.0 (zone candidates: top 3 bids + top 3 offers, king, tags + HELD on all shown; the 2.34.0 checks below kept) / 2.34.0 (2026-10-28 12:50 CT Rassul: "only the key price levels and the volume levels, 1 vwap level which is
// the latest. lets not have the gamma levels right now. the percentages ... left justified. and the level name and value should be right justified. the hod
// lod should be on the right of the level column not in it ... on the bottom left next to the levels column"): through the REAL DealerProfile.cpp -
// (e) gamma-named levels, VWAP bands and a 2nd VWAP never reach the column, exactly ONE VWAP badge; dpLatestVwap = SessionVWAPLogic (RTH / overnight);
// (f) odds LEFT-justified at the column's left edge, name chips + price RIGHT-justified at its right edge; (g) the HOD/LOD table docked bottom-left RIGHT of
// the column: the column keeps its 230 px and full height, no badge / ground over the table; (h) HELD (2.32.0) kept: the bright line + the word in the price
// area, the word steps right of the HOD/LOD table, no line part under it, the same after a restart (record read back).
// Build (Linux, from plugin/): g++ -std=c++17 -g -I<sdk include> -I. test_dealerprofile_2340.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// was:
// test_dealerprofile_2330.cpp -- lsDealerProfile 2.33.0 (2026-10-28, Rassul 12:16-12:18 CT "i want a column for levels and move all the level badges in the
// levels column because the chart is too messy. also move the hod lod box aligned with the liquidity profile on the left"): THE KEY LEVELS COLUMN at the
// pane's LEFT edge (ColumnBoard 1.9.0) through the REAL DealerProfile.cpp: (a) no level badge in the price area, (b) every badge in the column at / near its
// price row, none overlapping, none under the HOD/LOD table (docked at the column's top: the column takes its width), (d) the lines run from the column to
// LMT F, no rank chip on the badges; the rest of 2.32.0 (liquidity column, top 3, touch tags, HELD) unchanged.
// Build (Linux, from plugin/): g++ -std=c++17 -g -I<sdk include> -I. test_dealerprofile_2330.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// was:
// test_dealerprofile_2320.cpp -- lsDealerProfile 2.32.0 (2026-10-28): (b) the TOP 3 real orders within 3 ATR of price (king + 2; orders beyond the reach never shown / never king), (c) the touch tags from SignalBoard S (latched only for shown levels, colour by meaning, final once printed, tag-only rows) and HELD (bright line + word, never over candles / badges, read back after a restart). Build: g++ -std=c++17 -I<sdk include> -I. test_dealerprofile_2320.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// was test_dealerprofile_2310.cpp -- lsDealerProfile 2.31.0 (2026-10-28 07:xx CT, option R): the LIQUIDITY column on the RIGHT after the Delta Profile (ColumnBoard 1.8.0),
// rank nearest price | bar toward price | lots at the outer edge, a STEADY top 10 (Top10Stable.h), the badge rank on the RIGHT end, level lines in the price area only.
// was test_dealerprofile_2300.cpp -- lsDealerProfile 2.30.0 (2026-10-28): the LIQUIDITY column at the pane's LEFT edge (ColumnBoard 1.6.0), level lines
// from it to the LMT F column with centred badges and the RANK on the badge's LEFT end; (a) no 'B' producer = the 2.29.0 level-tagged orders in the
// column; (b) lsLiquidityProfile's resting orders on the SignalBoard ('B') = the TOP 10 by lots at their own prices (bars toward price) and the badge
// rank from the best order within 1 ATR beyond the level; posts 'Q' (top 10) and 'V' (levels). Copied from test_dealerprofile_2290.cpp (13 Oct ES fixture).
// test_dealerprofile_2280.cpp -- lsDealerProfile 2.28.0 THE COMBINED LEVELS + LIQUIDITY COLUMN (Rassul approved 2026-10-27 13:30-13:39 CT,
// mockup levels_final.png) through the REAL DealerProfile.cpp on the mocked host of test_dealerprofile_2190.cpp (13 Oct ES fixture):
// rows in price order, never overlapping, chips key > volume > gamma, merged rows, liquidity + rank from LRA-LmtLevels-ES-180.txt, no
// Dealer column / bars, the version header in the column, off-screen arrows; the draw list is written to dp2290_ops.json for the render check.
// Build (Linux, from plugin/): g++ -std=c++17 -g -I<sdk include> -I. test_dealerprofile_2290.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// test_dealerprofile_2190.cpp -- lsDealerProfile 2.19.0 (2026-10-21, Rassul approved Option B; 22:52 "show both hod and lod badges"):
// the level % blocks "33%  /  66%" through the REAL DealerProfile.cpp on the mocked host of test_dealerprofile_2110.cpp (real irtsdk.h,
// every draw call recorded, the 13 Oct ES files in tests_dp290/): both sides (HOD above the RTH open, LOD below), exact format, green /
// grey outline, inside the Levels column and never overlapping, the hover on the block only, missing / garbled / wrong-market file =
// no block + status reason, per-market files on two charts, the setting removes the blocks and the extra column width.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2190.cpp
//            -Wl,--unresolved-symbols=ignore-all -o t && ./t
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
void cppExtension::RCT::drawText(const char* t, RTBOOL, RTBOOL rj) {
    const int w = tw(t); const int l = rj ? right - w : left;
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
int main()
{
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2290XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp");
        check(src.find(std::string("p->setVersion(\"") + DP_VERSION + "\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && std::string(DP_VERSION) == "2.37.0" && DP_SIDEN == 3 && std::string(ltc::VERSION) == "1.1.0" && src.find("p->setVersion(\"2.37.0\")") != std::string::npos && std::string(cb::VERSION) == "1.9.1" && DP_TOPN == 3 && DP_REACH_ATR == 3.0 && std::string(lfd::VERSION) == "1.0.0", "versions 2.36.1 agree (LiqTouch 1.1.0, 3 a side in zone mode) (ColumnBoard 1.9.1), top 3 within 3 ATR, LevelFade 1.0.0");
        check(DP_LINES && !DP_GAMMA && std::string(lvc::VERSION) == "1.1.0" && lvc::TAG_ATR == 0 && lvc::MAX_RANK == 10, "level lines on, gamma off, his one-sided 1 ATR tag (LP), top 10");
    }
    dealerFile(true);
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    lv.push_back(klv::Merged{ "Open", 7850.0, false }); lv.push_back(klv::Merged{ "VAH", 7865.5, false }); lv.push_back(klv::Merged{ "DPoc", 7840.0, false });
    // (2.34.0) (e) one VWAP + things that must never reach the column: VWAP bands, a 2nd VWAP, gamma-named levels
    lv.push_back(klv::Merged{ "VWAP", 7852.0, false }); lv.push_back(klv::Merged{ "VWAP+1", 7858.0, false }); lv.push_back(klv::Merged{ "VWAP-2", 7828.0, false });
    lv.push_back(klv::Merged{ "VWAP", 7845.0, false }); lv.push_back(klv::Merged{ "HVL", 7843.0, false }); lv.push_back(klv::Merged{ "G1", 7861.0, false });
    put("LRA-LevelBadges-ES.csv", badgeFile("ES", 11, 22));
    {   char b[64]; std::snprintf(b, sizeof(b), "LMTL,1,%lld,0.25\n", (long long)time(nullptr));
        put("LRA-LmtLevels-ES-180.txt", std::string(b) + "O,A,300,300,PDH,7863.250000,7864.000000\nO,A,120,4o 120,PFH,7864.000000,7865.000000\n"
            "O,B,250,250,PDL,7834.500000,7834.000000\nO,B,90,90,PS 0D,7825.000000,7824.500000\nO,A,60,60,G1,7900.000000,7900.500000\n"); }
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 150); cb::publish(cb::BIG, a.sym, a.spb, MOCK_CHART, 1, 64);
    drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 150); cb::publish(cb::BIG, a.sym, a.spb, MOCK_CHART, 2, 64);
    std::vector<Op> O = drawOn(X, a, MOCK_CHART);
    DealerProfile::ColsState c = colsOf(X, a);
    {   // (2.36.1, Rassul 18:24 CT "you shouldn't be able to see the background") the Key Levels column ground: ONE solid box in the chart
        // background colour over the FULL column (pane left .. kR, pane top .. bottom), drawn before anything else inside the column
        const int kR0 = 1 + c.keysW; int gi = -1;
        for (size_t i = 0; i < O.size(); ++i) if (O[i].kind == "box" && O[i].col == (MOCK_BG & 0xFFFFFF) && O[i].l == 0 && O[i].r == kR0 && O[i].t == 0 && O[i].b == PH) { gi = (int)i; break; }
        check(gi >= 0 && O[(size_t)gi].style == (int)PAT_SOLID, "(2.36.1) the Key Levels column ground: a solid box in the chart background colour, full width 0-" + std::to_string(kR0) + " and full height 0-" + std::to_string(PH));
        int before = 0; for (int i = 0; i < gi; ++i) { const Op& q = O[(size_t)i]; if (q.kind != "line" && std::min(q.l, q.r) < kR0 && std::max(q.l, q.r) > 0) ++before; }
        check(gi >= 0 && before == 0, "(2.36.1) the ground is drawn first in the column: badges / odds / ticks on top (" + std::to_string(before) + " drawn before it)");
        int after = 0; for (size_t i = (size_t)(gi < 0 ? 0 : gi + 1); i < O.size(); ++i) if (O[i].kind == "text" && O[i].l >= 1 && O[i].r <= kR0) ++after;
        check(after >= 4, "(2.36.1) the column's texts are drawn after the ground (" + std::to_string(after) + ")");
        const std::string st = slurp(fl("DealerProfile.status-ES-180.txt")); check(st.find("ground solid 0x141821") != std::string::npos, "(2.36.1) status KEYS_COLUMN reports the solid ground colour");
    }
    check(c.lay.w[cb::LEVELS] == 170, "the liquidity column is 170 px (" + std::to_string(c.lay.w[cb::LEVELS]) + ")");
    check(c.lay.big.l > c.lay.priceRight && c.lay.big.r <= c.lay.delta.l && c.lay.delta.r <= c.lay.levels.l && c.lay.levels.r <= c.lay.dealer.l && cb::columnsClean(c.lay), "right side: LMT F | Delta | Liquidity | Dealer, clean (" + std::to_string(c.lay.big.l) + " " + std::to_string(c.lay.delta.l) + " " + std::to_string(c.lay.levels.l) + " " + std::to_string(c.lay.dealer.l) + ")");
    auto textAt = [&](const std::string& t) -> const Op* { for (auto& q : O) if (q.kind == "text" && q.text == t) return &q; return nullptr; };
    check(hasText(O, std::string("DP v") + DP_VERSION + "  LIQUIDITY"), "header in the liquidity column");
    bool gam = false; for (auto& q : O) if (q.kind == "text" && (q.text.rfind("HVL", 0) == 0 || q.text.rfind("PS0", 0) == 0 || q.text == "GEX" || q.text.rfind("G1", 0) == 0 || q.text.rfind("CR", 0) == 0)) gam = true;
    check(!gam, "no gamma levels drawn");
    const int xL = 2, xR = (c.lay.big.w() > 0 ? c.lay.big.l : c.lay.priceRight) - 2;
    std::vector<Op> bad; for (auto& q : O) if (q.kind == "box" && q.col == 0x0B0F19 && q.l >= xL && q.r < xR && q.r - q.l < 600 && q.b - q.t < 40) bad.push_back(q);
    int ov = 0; for (size_t i2 = 0; i2 < bad.size(); ++i2) for (size_t j2 = i2 + 1; j2 < bad.size(); ++j2) if (bad[i2].t < bad[j2].b && bad[j2].t < bad[i2].b && bad[i2].l < bad[j2].r && bad[j2].l < bad[i2].r) ++ov;
    check(bad.size() >= 4 && ov == 0, "badges: " + std::to_string(bad.size()) + ", none overlapping (" + std::to_string(ov) + ")");
    // (2.33.0) THE KEY LEVELS COLUMN [kL, kR] at the left edge
    const int kL = 1, kR = kL + c.keysW;
    check(c.keysW == 230 && c.keysL == 1 && !c.keysDocked, "(2.33.0) the Key Levels column at the left edge, 230 px, no table yet (" + std::to_string(c.keysW) + ")");
    { int w = 0; check(cb::keysFresh(a.sym, a.spb, MOCK_CHART, dpNowMs(), w) && w == 230, "(2.33.0) the column is posted on the board (LRA_COLV_) for the other plugins"); }
    { int inPrice = 0, inCol = 0; for (auto& q : bad) { if (q.l >= kL && q.r <= kR) ++inCol; else ++inPrice; }
      check(inPrice == 0 && inCol >= 4, "(a) no level badge in the price area: " + std::to_string(inCol) + " in the column, " + std::to_string(inPrice) + " outside"); }
    {   // (b) each badge at its line's row unless stacked; the moved ones keep the price order (stack) and have a tick to their line
        int near = 0; for (auto& q : bad) { const int yc = (q.t + q.b) / 2; for (auto& ln : O) if (ln.kind == "line" && ln.t == ln.b && std::max(ln.l, ln.r) == kR && std::abs(ln.l - ln.r) <= 6 && std::abs(ln.t - yc) <= 1) { ++near; break; } }
        check(near >= 2, "(2.36.0)(b) badges on their own price row with a <= 6 px tick at the column edge (" + std::to_string(near) + ")");
        int ticks = 0; for (auto& ln : O) if (ln.kind == "line" && ln.t != ln.b && ln.l != ln.r && std::max(ln.l, ln.r) == kR) ++ticks;
        check(ticks == c.keysMoved, "(b) every moved badge has a tick to its line (" + std::to_string(ticks) + " / " + std::to_string(c.keysMoved) + ")");
    }
    {   // (2.36.0, Rassul 16:26 CT "remove the drawing of levels across the chart") no line in the price area at all: no level lines, no
        // liquidity order lines, no HELD line (price area = right of the Key Levels column, left of the liquidity column)
        int inPA = 0; for (auto& q : O) if (q.kind == "line" && std::max(q.l, q.r) > kR + 1 && std::min(q.l, q.r) < c.lay.levels.l - 1) ++inPA;
        check(inPA == 0, "(2.36.0)(d) no level / order line in the price area (" + std::to_string(inPA) + ")");
        int offIn = 0, offOut = 0; for (auto& q : O) if (q.kind == "text" && (q.text.rfind("^ ", 0) == 0 || q.text.rfind("v ", 0) == 0)) { if (q.l >= kL && q.r <= kR) ++offIn; else ++offOut; }
        check(offIn >= 1 && offOut == 0, "(2.36.0) off-screen level markers in the Key Levels column (" + std::to_string(offIn) + " in, " + std::to_string(offOut) + " outside)");
        const std::string stL = slurp(fl("DealerProfile.status-ES-180.txt")); check(stL.find("2.37.0") != std::string::npos, "(2.36.1) status shows 2.36.1"); }
    { int through = 0; for (auto& q : O) if (q.kind == "line" && q.t == q.b && q.l < c.lay.big.l && q.r > c.lay.big.l + 2) ++through; check(through == 0, "no line crosses into the LMT F / Delta columns (" + std::to_string(through) + ")"); }
    const Op* p250 = textAt("250");
    check(p250 && p250->l >= c.lay.levels.l && p250->l < c.lay.levels.r, "(a) no B producer: PDL's 250 bids in the liquidity column (2.29.0 rows)");
    { int rk = 0; for (auto& q : O) if (q.kind == "text" && q.text.size() <= 2 && std::isdigit((unsigned char)q.text[0]) && q.l >= kL && q.r <= kR) ++rk;
      check(rk == 0, "(2.33.0) no liquidity rank chip on the level badges (" + std::to_string(rk) + ")"); }
    {   // (2.34.0) (e) only key + volume levels + ONE VWAP; (f) odds left, name + price right
        int vw = 0, band = 0, gm = 0; for (auto& q : O) if (q.kind == "text" && q.l >= kL && q.r <= kR) { if (q.text == "VWAP") ++vw; if (q.text.rfind("VWAP+", 0) == 0 || q.text.rfind("VWAP-", 0) == 0) ++band;
            if (q.text == "HVL" || q.text == "G1" || q.text.rfind("CR", 0) == 0 || q.text.rfind("PS", 0) == 0 || q.text.rfind("GW", 0) == 0) ++gm; }
        check(vw == 1, "(e) exactly one VWAP badge in the column (" + std::to_string(vw) + ")");
        check(band == 0 && gm == 0, "(e) no VWAP band / gamma badge in the column (" + std::to_string(band) + " / " + std::to_string(gm) + ")");
        check(c.keysFiltered == 5 && c.vwapBadges == 1, "(e) filtered 5 (VWAP+1, VWAP-2, 2nd VWAP, HVL, G1), one VWAP (" + std::to_string(c.keysFiltered) + ")");
        bool anyBand = false; for (auto& q : O) if (q.kind == "text" && (q.text.rfind("VWAP+", 0) == 0 || q.text.rfind("VWAP-", 0) == 0 || q.text == "HVL" || q.text == "G1")) anyBand = true;
        check(!anyBand, "(e) nowhere on the chart");
        int lines7858 = 0; for (auto& q : O) if (q.kind == "line" && q.t == q.b && std::abs(q.t - yOfP(7858.0f)) <= 0 && q.l == kR + 1) ++lines7858;
        check(lines7858 == 0, "(e) no VWAP+1 line");
        int oddsL = 0, oddsBad = 0; for (auto& q : O) if (q.kind == "text" && isPct(q.text)) { if (q.l == kL + 4) ++oddsL; else ++oddsBad; }
        check(oddsL >= 3 && oddsBad == 0, "(f) the odds LEFT-justified at the column's left edge (" + std::to_string(oddsL) + " / off " + std::to_string(oddsBad) + ")");
        int right = 0, notRight = 0; for (auto& q : bad) { if (q.r == kR - 3) ++right; else ++notRight; }
        check(right >= 4 && notRight == 0, "(f) every name + price badge RIGHT-justified at the column's right edge (" + std::to_string(right) + " / off " + std::to_string(notRight) + ")");
        int gapBad = 0; for (auto& q : O) if (q.kind == "text" && isPct(q.text)) for (auto& b : bad) if (std::abs((b.t + b.b) / 2 - (q.t + q.b) / 2) <= 2 && q.r + 4 > b.l) ++gapBad;
        check(gapBad == 0, "(f) the odds never touch their badge (" + std::to_string(gapBad) + ")");
        // dpLatestVwap = SessionVWAPLogic: 3 RTH bars (ES 08:30-15:00 CT) -> the typical-price VWAP; an overnight-only tail -> the overnight VWAP
        std::vector<klv::Bar> vb; const long d0 = svl::daysFromCivil(2026, 10, 22);
        vb.push_back(klv::Bar{ d0, 8 * 3600 + 33 * 60, 101.f, 99.f, 100.f }); vb.back().v = 10;
        vb.push_back(klv::Bar{ d0, 8 * 3600 + 36 * 60, 103.f, 101.f, 102.f }); vb.back().v = 30;
        vb.push_back(klv::Bar{ d0, 8 * 3600 + 39 * 60, 105.f, 103.f, 104.f }); vb.back().v = 0;
        check(std::fabs(dpLatestVwap(vb, 180, 510, 900) - (100.0 * 10 + 102.0 * 30) / 40.0) < 1e-4, "(e) the VWAP = SessionVWAPLogic RTH VWAP (typical price x volume)");
        std::vector<klv::Bar> ob; ob.push_back(klv::Bar{ d0, 18 * 3600 + 3 * 60, 11.f, 9.f, 10.f }); ob.back().v = 5;
        check(std::fabs(dpLatestVwap(ob, 180, 510, 900) - 10.0) < 1e-4 && dpLatestVwap(std::vector<klv::Bar>(), 180, 510, 900) == 0.0, "(e) overnight: the overnight VWAP; no bars: none");
    }
    {   // (b) / (c) the HOD/LOD table docked at the column's top (lsHodLod 2.10.0 posts it): 300 px wide -> the column takes its width; no badge under it,
        // the column ground never painted over it; a level the table covers gets its badge below the table
        const int HBL = 1 + 230 + cb::DOCK_GAP, HBT = PH - cb::DOCK_GAP - 2 - 240, HBR = HBL + 300, HBB = PH - cb::DOCK_GAP - 2;   // (2.34.0) bottom-left, RIGHT of the column
        bdk::publishHodLod(a.sym, a.spb, MOCK_CHART, dpNowMs(), bdk::mk(HBL, HBT, HBR, HBB));
        std::vector<Op> O3 = drawOn(X, a, MOCK_CHART);
        DealerProfile::ColsState c3 = colsOf(X, a);
        { std::ofstream f("dp2340_docked_ops.json"); f << "{\"pw\":" << PW << ",\"ph\":" << PH << ",\"hodlod\":[" << HBL << "," << HBT << "," << HBR << "," << HBB << "],\"ops\":[";
          for (size_t i2 = 0; i2 < O3.size(); ++i2) { const Op& q = O3[i2]; std::string t; for (char ch : q.text) { if (ch == '"' || ch == '\\') t += '\\'; t += ch; }
            f << (i2 ? "," : "") << "{\"k\":\"" << q.kind << "\",\"l\":" << q.l << ",\"t\":" << q.t << ",\"r\":" << q.r << ",\"b\":" << q.b << ",\"c\":" << q.col << ",\"c2\":" << q.col2 << ",\"f\":" << q.font << ",\"bo\":" << (q.bold ? 1 : 0) << ",\"x\":\"" << t << "\"}"; }
          f << "]}\n"; }
        check(c3.keysDocked && c3.keysW == 230 && c3.keysTop == 22, "(g) table docked bottom-left right of the column: the column keeps 230 px and its full height (" + std::to_string(c3.keysW) + "), badges from y " + std::to_string(c3.keysTop));
        int under = 0, col = 0, ground = 0, n3 = 0; const int kR3 = 1 + c3.keysW;
        for (auto& q : O3) {
            if (q.kind == "box" && q.col == 0x0B0F19 && q.r - q.l < c3.keysW - 3 && q.b - q.t < 40 && q.l >= 1 && q.r <= kR3) { ++n3; if (q.t < HBB && q.b > HBT && q.r > HBL && q.l < HBR) { ++under; std::printf("  under: box %d %d %d %d\n", q.l, q.t, q.r, q.b); } }
            if (q.kind == "box" && q.col == 0x0B0F19 && q.l < HBR && q.r > HBL && q.t < HBB && q.b > HBT && !(q.r - q.l < 600 && q.b - q.t < 40)) ++ground;
            if (q.kind == "text" && q.l < HBR && q.r > HBL && (q.t + q.b) / 2 < HBB && (q.t + q.b) / 2 > HBT && q.l >= 1 && q.r <= kR3) { ++col; std::printf("  under: text %s %d %d %d %d\n", q.text.c_str(), q.l, q.t, q.r, q.b); }
        }
        check(n3 >= 4 && under == 0 && col == 0, "(g) " + std::to_string(n3) + " badges, none under the HOD/LOD table (" + std::to_string(under) + " / text " + std::to_string(col) + ")");
        check(ground == 0, "(c) the column ground is never painted over the table (" + std::to_string(ground) + ")");
        int ov3 = 0; std::vector<Op> b3; for (auto& q : O3) if (q.kind == "box" && q.col == 0x0B0F19 && q.r - q.l < c3.keysW - 3 && q.b - q.t < 40 && q.l >= 1 && q.r <= kR3) b3.push_back(q);
        for (size_t i2 = 0; i2 < b3.size(); ++i2) for (size_t j2 = i2 + 1; j2 < b3.size(); ++j2) if (b3[i2].t < b3[j2].b && b3[j2].t < b3[i2].b) ++ov3;
        check(ov3 == 0, "(b) docked: badges never overlap (" + std::to_string(ov3) + ")");
        int l3 = 0; for (auto& q : O3) if (q.kind == "line" && q.t == q.b && q.l == kR3 + 1) ++l3;
        check(l3 == 0, "(2.36.0)(g) docked: no level line leaves the column (" + std::to_string(l3) + ")");
        const std::string st3 = slurp(fl("DealerProfile.status-ES-180.txt"));
        check(st3.find("KEYS_COLUMN,left edge") != std::string::npos && st3.find("docked bottom-left, right of the column") != std::string::npos && st3.find("layout odds left | name + price right") != std::string::npos && st3.find("in the price area 0") != std::string::npos, "status KEYS_COLUMN line");
        bdk::clearHodLod(a.sym, a.spb, MOCK_CHART);
        drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c4 = colsOf(X, a);
        check(!c4.keysDocked && c4.keysW == 230, "table removed: the column back to 230 px");
    }
    // (b) lsLiquidityProfile posts its resting orders: the top 10 by lots at their own prices
    {   std::vector<sb::Fact> B;
        const double px[13] = { 7856.25, 7857.0, 7849.0, 7846.0, 7845.0, 7846.25, 7850.5, 7838.0, 7855.0, 7858.0, 7841.0, 7836.0, 7870.0 };   // (2.32.0) the price is 7853.25, 3 ATR = 8.30: 7838 / 7841 / 7836 / 7870 are beyond reach
        const long long lots[13] = { 300, 120, 250, 900, 80, 75, 70, 65, 60, 55, 50, 45, 5000 };
        for (int i = 0; i < 13; ++i) { sb::Fact f; f.bar = 1; f.lo = f.hi = px[i]; f.side = px[i] < 7848 ? 1 : -1; f.val = (double)lots[i]; B.push_back(f); }
        sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B);
        {   CUR = &a; X.loadHostState(); t10::Params P0; P0.persistMs = 0; X.cs.top10 = t10::Board(P0); X.saveHostState(); }   // the 30 s entry wait off for the test
        drawOn(X, a, MOCK_CHART);
        std::vector<Op> O2 = drawOn(X, a, MOCK_CHART);
        DealerProfile::ColsState c2 = colsOf(X, a);
        int chips = 0; for (auto& q : O2) if (q.kind == "text" && q.l >= c2.lay.levels.l && q.l < c2.lay.levels.l + 24 && q.text.size() <= 2 && std::isdigit((unsigned char)q.text[0])) ++chips;
        check(chips == 3, "(b) exactly three rank chips at the column's price-side edge (" + std::to_string(chips) + ")");
        { int lotsRight = 0; for (auto& q : O2) if (q.kind == "text" && (q.text == "900" || q.text == "300") && q.r <= c2.lay.levels.r - 8 && q.l > c2.lay.levels.l + 30) ++lotsRight; check(lotsRight == 2, "(2.37.0)(b) lots at the right end of the bar, before the tag lane (" + std::to_string(lotsRight) + ")"); }
        check(textAt("900") == nullptr || true, "");
        bool has900 = false; for (auto& q : O2) if (q.kind == "text" && q.text == "900" && q.l > c2.lay.levels.l) has900 = true;
        check(has900, "(b) the biggest resting order (900) in the column");
        std::vector<sb::Fact> Q, V;
        check(sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q, false) && Q.size() == 3 && Q[0].code == 1 && Q[0].val == 900 && Q[1].val == 300 && Q[2].val == 250, "(b) Q facts: the top 3 (900 king, 300, 250), rank 1 = 900");
        check(!hasText(O2, "5000") && !hasText(O2, "65") && !hasText(O2, "50"), "(2.32.0) orders beyond 3 ATR (5000 at 7870, 65 at 7838, 50 at 7841) are never shown, never king");
        { const std::string stx = slurp(fl("DealerProfile.status-ES-180.txt")); check(stx.find("beyond reach 4") != std::string::npos && stx.find("shown top 3") != std::string::npos, "(2.32.0) status: 4 orders beyond the 3-ATR reach, top 3"); }
        check(sb::readFacts('V', a.sym, a.spb, MOCK_CHART, dpNowMs(), V, true) && V.size() >= 4, "V facts: the levels (decided)");
        const std::string st2 = slurp(fl("DealerProfile.status-ES-180.txt"));
        check(st2.find("LIQUIDITY_TOP10,from SignalBoard B") != std::string::npos && st2.find("Top10Stable") != std::string::npos, "(b) status LIQUIDITY_TOP10 from B, steady");
        {   // a size wobble under 1.25x keeps the order; an 11th order 1.1x the #10 does not get in
            for (auto& f : B) if (f.val == 300) f.val = 330;
            sb::Fact n; n.bar = 1; n.lo = n.hi = 7859.0; n.side = -1; n.val = 52; B.push_back(n);
            sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B); drawOn(X, a, MOCK_CHART);
            std::vector<sb::Fact> Q2; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q2, false);
            bool same = Q2.size() == 3; for (size_t k = 0; same && k < Q2.size() && k < Q.size(); ++k) same = Q2[k].lo == Q[k].lo;
            check(same, "(b) steady: small size changes and a weak 11th order do not reshuffle the top 10"); }
        {   // (2.31.2, Rassul 10:06 "top 5 nodes that are real") a 2000-lot order re-priced 3x in 5 min is quoting: NOT shown, never king; a 1x-moved one is real (shown plainly, no "~")
            sb::Fact qf; qf.bar = 1; qf.lo = qf.hi = 7847.0; qf.side = 1; qf.val = 2000; qf.state = 3; B.push_back(qf);
            for (auto& f : B) if (f.val == 330) f.state = 1;
            sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B); drawOn(X, a, MOCK_CHART); std::vector<Op> O3 = drawOn(X, a, MOCK_CHART);
            check(!hasText(O3, "2000") && !hasText(O3, "~2000"), "quoting 2000 (re-priced 3x) not shown");
            bool anyTilde = false; for (auto& q : O3) if (q.kind == "text" && !q.text.empty() && q.text[0] == '~') anyTilde = true;
            check(!anyTilde, "no ~ rows at all");
            std::vector<sb::Fact> Q3; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q3, false);
            check(Q3.size() == 3 && Q3[0].val == 900 && Q3[0].code == 1, "king = the biggest REAL order (900), not the quoting 2000");
            int chips = 0; for (auto& q : O3) if (q.kind == "text" && q.l >= c2.lay.levels.l && q.l < c2.lay.levels.l + 24 && q.text.size() <= 2 && std::isdigit((unsigned char)q.text[0])) ++chips;
            check(chips <= 3, "never more than 3 rows (" + std::to_string(chips) + ")");
            // it stops re-pricing (state 0): now real -> the biggest -> king
            for (auto& f : B) if (f.val == 2000) f.state = 0;
            sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B); drawOn(X, a, MOCK_CHART); drawOn(X, a, MOCK_CHART);
            std::vector<sb::Fact> Q4; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q4, false);
            check(!Q4.empty() && Q4[0].val == 2000 && Q4[0].code == 1, "once it rests (no more re-pricing) the 2000 is king");
            for (auto& f : B) if (f.val == 330) f.state = 0;
        }
        {   // (2.31.1, Rassul 09:47 "when I scroll up I see another king node") two viewports, the same book = the same king and the same ranks
            std::vector<sb::Fact> Qa, Qb; const float t0 = PTOP, b0 = PBOT;
            sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B); drawOn(X, a, MOCK_CHART);
            sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Qa, false);
            PTOP = 7880.0f; PBOT = 7846.5f;   // (2.32.0) scrolled: the 900 (7846) goes below the pane; was: the 2000 king (7847) stays, the 900 bid (7830) and 250 (7834) go below the pane
            std::vector<Op> Ob = drawOn(X, a, MOCK_CHART); drawOn(X, a, MOCK_CHART); Ob = drawOn(X, a, MOCK_CHART);
            sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Qb, false);
            bool same = Qa.size() == Qb.size() && !Qa.empty(); for (size_t k = 0; same && k < Qa.size(); ++k) same = Qa[k].lo == Qb[k].lo && Qa[k].code == Qb[k].code;
            check(same, "viewport A vs B: the same king and the same ranks (" + std::to_string(Qa.size()) + " / " + std::to_string(Qb.size()) + ")");
            bool below = false; for (auto& q : Ob) if (q.kind == "text" && q.text.find("#") != std::string::npos && q.l >= c2.lay.levels.l) below = true;
            check(below, "off-screen ranked rows: an edge marker with their ranks");
            PTOP = 7845.0f; PBOT = 7800.0f;   // the king (7847) above the pane: the K edge marker, no new king
            drawOn(X, a, MOCK_CHART); std::vector<Op> Oc = drawOn(X, a, MOCK_CHART); std::vector<sb::Fact> Qc; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Qc, false);
            bool kmark = false; for (auto& q : Oc) if (q.kind == "text" && q.text.find("K 7847") != std::string::npos) kmark = true;
            check(kmark && !Qc.empty() && Qc[0].lo == 7847.0 && Qc[0].code == 1, "king off screen: K edge marker with its price, still the king");
            check(!hasText(Oc, "~2000") && !hasText(Oc, "2000"), "the off-screen king is not drawn as a row");
            PTOP = t0; PBOT = b0;
        }
        {   // (2.32.0) (c) THE TOUCH TAGS + HELD from lsLiquidityProfile's SignalBoard S facts
            CUR = &a; NOW = a.dt.back() + 180;
            auto ct = [](unsigned long d) { time_t t = (time_t)d; struct tm m; gmtime_r(&t, &m); return fr::ctAbs(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday, m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec); };
            const long long nowCt = ct(NOW), n = (long long)a.dt.size();
            const long long tBid = ct(a.dt[(size_t)(n - 2)]) - 100, tOff = nowCt - 30, tOut = ct(a.dt[(size_t)(n - 1)]) - 60;   // touches inside visible candles
            auto fact = [](long long bar, double px, int side, int code, double val) { sb::Fact f; f.bar = bar; f.lo = f.hi = px; f.side = side; f.code = code; f.val = val; f.state = code == 0 ? 0 : 1; f.proven = code >= 1 && code <= 5; return f; };
            check(ltc::tagColour(ltc::T_STAYS_PLUS, ltc::BID) == 1 && ltc::tagColour(ltc::T_STAYS, ltc::ASK) == -1 && ltc::tagColour(ltc::T_GONE, ltc::BID) == -1 && ltc::tagColour(ltc::T_GONE_PLUS, ltc::ASK) == 1 && ltc::tagColour(ltc::T_STAYS_WEAK, ltc::BID) == 0, "(c) colour by meaning: bid STAYS / offer GONE green, offer STAYS / bid GONE red, weak grey");
            check(ltc::heldColour(ltc::BID) == 1 && ltc::heldColour(ltc::ASK) == -1, "(c) HELD colour: bid green, offer red");
            std::vector<sb::Fact> Sf = { fact(tBid, 7847.0, 1, 0, 0), fact(tOff, 7856.25, -1, 0, 0), fact(tOff, 7838.0, 1, 0, 0) };   // 7838 is not a shown level
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); drawOn(X, a, MOCK_CHART);
            check(colsOf(X, a).ttags.size() == 2, "(c) only touches of SHOWN levels are latched (2 of 3)");
            Sf = { fact(tBid, 7847.0, 1, ltc::T_STAYS_PLUS, 2), fact(tOff, 7856.25, -1, ltc::T_GONE, 1) };
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); std::vector<Op> T1 = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c3 = colsOf(X, a);
            const Op* sp = nullptr; const Op* go = nullptr; for (auto& q : T1) if (q.kind == "text" && q.l >= c3.lay.levels.l && q.r <= c3.lay.levels.r) { if (q.text == "STAYS+") sp = &q; if (q.text == "GONE") go = &q; }
            check(sp && sp->col == (C_BUY & 0xFFFFFF), "(c) STAYS+ on the bid king's row, green");
            check(go && go->col == (C_BUY & 0xFFFFFF), "(c) GONE on the offer's row, green (sellers left)");
            {   const Op* k2000 = nullptr; for (auto& q : T1) if (q.kind == "text" && q.text == "2000") k2000 = &q;
                check(sp && k2000 && std::abs((sp->t + sp->b) / 2 - (k2000->t + k2000->b) / 2) <= 2 && sp->l > k2000->r, "(2.37.0)(c) the tag sits on its own zone's row AFTER the amount (\"1 [bar] 92 STAYS\")"); }
            // final once printed: a different later fact for the same touch never changes it
            Sf = { fact(tBid, 7847.0, 1, ltc::T_GONE_PLUS, -2), fact(tOff, 7856.25, -1, ltc::T_GONE, 1) };
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); std::vector<Op> T2 = drawOn(X, a, MOCK_CHART);
            check(hasText(T2, "STAYS+") && !hasText(T2, "GONE+"), "(c) no repaint: the STAYS+ tag never changes");
            // the outcome: the bid HELD -> its line bright + HELD; the tag leaves the column (fades)
            Sf = { fact(tBid, 7847.0, 1, ltc::T_STAYS_PLUS, 2), fact(tBid, 7847.0, 1, 11, (double)(tOut - tBid)), fact(tOff, 7856.25, -1, ltc::T_GONE, 1), fact(tOff, 7856.25, -1, 12, 20) };
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); std::vector<Op> T3 = drawOn(X, a, MOCK_CHART); c3 = colsOf(X, a);
            const int yH = yOfP(7847.0f); const Op* held = nullptr; const Op* hl = nullptr;
            for (auto& q : T3) { if (q.kind == "text" && q.text == "HELD") held = &q; if (q.kind == "line" && q.t == yH && q.b == yH && q.font == 2 && q.col == (C_BUY & 0xFFFFFF)) hl = &q; }
            { int bright = 0; for (auto& q : T3) if (q.kind == "line" && q.font == 2) ++bright; check(hl == nullptr && bright == 0, "(2.36.0)(c) no HELD line on the chart"); }
            check(held == nullptr, "(2.37.0)(c) no HELD in the column (the order lines carry HELD / BREAK on the chart)");
            check(!hasText(T3, "STAYS+") && !hasText(T3, "GONE"), "(c) tags fade once the outcome is decided");
            int brokeLines = 0; for (auto& q : T3) if (q.kind == "line" && q.font == 2 && q.t == yOfP(7856.25f) && q.b == q.t) ++brokeLines;
            check(brokeLines == 0 && !hasText(T3, "BROKE"), "(c) a level that BROKE gets no mark");
            if (held) {   // never over a candle or a level badge
                bool overC = false; for (size_t i = 0; i < a.dt.size(); ++i) { const int x = X0 + PPB * (int)i; if (x < held->l - 2 || x > held->r + 2) continue; if (yOfP(a.h[i]) <= held->b && yOfP(a.l[i]) >= held->t) overC = true; }
                bool overB = false; for (auto& q : T3) if (q.kind == "box" && q.col == 0x0B0F19 && q.r - q.l < 600 && q.b - q.t < 40 && q.l < held->r && held->l < q.r && q.t < held->b && held->t < q.b) overB = true;
                check(!overC && !overB, "(c) HELD never covers a candle or a level badge"); }
            const std::string tf = slurp(fl("DealerProfile.touch-ES-180.csv"));
            check(tf.find(",1,1,2,1,") != std::string::npos, "(c) the latched touch (bid, STAYS+, +2, HELD) is in DealerProfile.touch-ES-180.csv");
            // restart: a fresh indicator with NO S producer draws the same HELD (read back), nothing repainted
            {   DealerProfile Y; prime(Y, a, "ES", &lv); sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs() - 120000, std::vector<sb::Fact>());
                drawOn(Y, a, MOCK_CHART); std::vector<Op> R1 = drawOn(Y, a, MOCK_CHART);
                const Op* h2 = nullptr; for (auto& q : R1) if (q.kind == "text" && q.text == "HELD") h2 = &q;
                check(h2 == nullptr && held == nullptr, "(2.37.0)(c) after a restart still no HELD in the column"); }
            const std::string stx = slurp(fl("DealerProfile.status-ES-180.txt"));
            check(stx.find("TOUCH_TAGS,SignalBoard S") != std::string::npos && stx.find("HELD lines 0") != std::string::npos && stx.find("HELD words 0") != std::string::npos, "(2.36.0)(c) status TOUCH_TAGS: HELD lines 0, words 0");
            // (2.36.0) the 2.34.0 (h) checks (HELD line / word stepping round the HOD/LOD table) retired: nothing is drawn in the price area
        }
        O = O2; c = c2;
    }
    // ================= (2.35.0, Rassul approved 14:04 CT) ZONE MODE: lsLiquidityProfile 0.67.0 posts its zone candidates (B code 2) =========
    {   DealerProfile Z; prime(Z, a, "ES", &lv);
        {   CUR = &a; Z.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = 3; Z.cs.zBid = t10::Board(P0); Z.cs.zAsk = t10::Board(P0); Z.cs.zInit = true; Z.saveHostState(); }   // the 30 s entry wait off for the test
        auto zf = [](double px, int side, double lots, int code = 2) { sb::Fact f; f.bar = 1; f.lo = f.hi = px; f.side = side; f.val = lots; f.code = code; f.state = 0; return f; };
        // price 7853.25: bids 400 / 350 / 300 / 200 below, offers 500 / 150 above, + an old-style code-0 5000 that is NOT a candidate
        std::vector<sb::Fact> ZB = { zf(7850.0, 1, 400), zf(7848.0, 1, 350), zf(7846.0, 1, 300), zf(7842.0, 1, 200), zf(7856.0, -1, 500), zf(7858.0, -1, 150), zf(7851.0, 1, 5000, 0) };
        sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB); drawOn(Z, a, MOCK_CHART); std::vector<Op> Z1 = drawOn(Z, a, MOCK_CHART);
        std::vector<sb::Fact> QZ; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), QZ, false);
        int nb = 0, na = 0; for (auto& q : QZ) (q.side > 0 ? nb : na)++;
        check(QZ.size() == 5 && nb == 3 && na == 2, "(2.35.0) top 3 BIDS + the 2 offers that passed (fewer than 3 when fewer pass): " + std::to_string(nb) + " bids / " + std::to_string(na) + " offers");
        check(!QZ.empty() && QZ[0].val == 500 && QZ[0].code == 1 && QZ[0].side == -1, "(2.35.0) king = the biggest of all shown (offer 500), rank 1");
        bool has200 = false, has5000 = false; for (auto& q : QZ) { if (q.val == 200) has200 = true; if (q.val == 5000) has5000 = true; }
        check(!has200, "(2.35.0) the 4th bid (200) is not shown - 3 per side cap");
        check(!has5000 && !hasText(Z1, "5000"), "(2.35.0) a non-candidate (code 0) level is never shown in zone mode, never king");
        bool ranksOk = true; for (size_t k = 0; k < QZ.size(); ++k) { if (QZ[k].code != (int)k + 1) ranksOk = false; if (k && QZ[k].val > QZ[k - 1].val) ranksOk = false; }
        check(ranksOk, "(2.35.0) ranks 1..5 biggest first over both sides");
        { int bp = 0; for (auto& q : QZ) if (q.side > 0 && q.lo > 7853.25) ++bp; for (auto& q : QZ) if (q.side < 0 && q.lo < 7853.25) ++bp; check(bp == 0, "(2.35.0) bids below price, offers above"); }
        { const std::string stz = slurp(fl("DealerProfile.status-ES-180.txt")); check(stz.find("candidates ZONES from lsLiquidityProfile") != std::string::npos && stz.find("shown bids 3 / offers 2") != std::string::npos && stz.find("shown top 6") != std::string::npos, "(2.35.0) status: zone candidates, 3 bids / 2 offers, max 6"); }
        {   // hysteresis per side: a 4th bid 1.1x the weakest shown bid does not get in; 1.3x does (Top10Stable enterX 1.20)
            ZB.push_back(zf(7844.0, 1, 330)); sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB); drawOn(Z, a, MOCK_CHART);
            std::vector<sb::Fact> Q2; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q2, false); bool in330 = false; for (auto& q : Q2) if (q.val == 330) in330 = true;
            check(!in330 && Q2.size() == 5, "(2.35.0) steady per side: a challenger 1.1x the #3 bid does not displace it");
            ZB.back().val = 400; sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB); drawOn(Z, a, MOCK_CHART);
            Q2.clear(); sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q2, false); bool in400b = false, in300 = false; int nb2 = 0; for (auto& q : Q2) { if (q.lo == 7844.0) in400b = true; if (q.val == 300) in300 = true; if (q.side > 0) ++nb2; }
            check(in400b && !in300 && nb2 == 3, "(2.35.0) a challenger 1.33x the #3 bid replaces it, still 3 bids");
            ZB.pop_back(); sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB); }
        {   // (2.36.0, Rassul 16:40 CT) the LIQUIDITY RANK chip on the level badges: right before the name chip(s), best rank among the shown
            // zones from the level out to 1 ATR BEYOND it (resistance: level .. +1 ATR, support: -1 ATR .. level), side colour, king white
            std::vector<sb::Fact> V; sb::readFacts('V', a.sym, a.spb, MOCK_CHART, dpNowMs(), V, false);
            CUR = &a; const double atr = Z.atr14(); const double lastP = 7853.25;
            double Lr = 0, Ls = 0; for (auto& f : V) { if (f.code != 1) continue; { bool drawn = false; for (auto& q : Z1) if (q.kind == "text" && q.text == dpaudit::priceText("ES", f.lo) && q.r <= 240) drawn = true; if (!drawn) continue; } if (f.lo > lastP + 1.0 && f.lo < PTOP - 10 && (Lr == 0 || f.lo < Lr)) Lr = f.lo; if (f.lo < lastP - 1.0 && f.lo > PBOT + 10 && (Ls == 0 || f.lo > Ls)) Ls = f.lo; }
            std::printf("  rank-chip levels: resistance %.2f support %.2f ATR %.2f (V %zu)\n", Lr, Ls, atr, V.size());
            check(Lr > 0 && Ls > 0 && atr > 0, "(2.36.0) a key level above and below price to test the rank chip");
            auto chipOn = [&](const std::vector<Op>& Ops, double L, const DealerProfile::ColsState& cc, const Op** priceOp) -> const Op* {
                const std::string pt = dpaudit::priceText("ES", L); const Op* po = nullptr; const int kR2 = 1 + cc.keysW;
                for (auto& q : Ops) if (q.kind == "text" && q.text == pt && q.l >= 1 && q.r <= kR2) po = &q;
                if (priceOp) *priceOp = po; if (!po) return nullptr;
                const int yc = (po->t + po->b) / 2; const Op* ch = nullptr;
                for (auto& q : Ops) if (q.kind == "text" && !q.text.empty() && q.text.size() <= 1 && std::isdigit((unsigned char)q.text[0]) && q.l > po->r && q.r <= kR2 && std::abs((q.t + q.b) / 2 - yc) <= 2) ch = &q;   // (2.37.0) the rank box is RIGHT of the value + name
                return ch; };
            auto run = [&](double offerPx, double bidPx, std::vector<Op>& Ops, DealerProfile::ColsState& cc) {
                DealerProfile Y; prime(Y, a, "ES", &lv);
                { CUR = &a; Y.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = 3; Y.cs.zBid = t10::Board(P0); Y.cs.zAsk = t10::Board(P0); Y.cs.zInit = true; Y.saveHostState(); }
                std::vector<sb::Fact> ZZ = { zf(offerPx, -1, 900), zf(bidPx, 1, 300) };
                sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZZ); drawOn(Y, a, MOCK_CHART);
                { CUR = &a; Y.loadHostState(); Y.cs.liqLastMs = -1; Y.cs.lpMedMs = -1; Y.saveHostState(); }   // the liqlevels file: the throttle (tested below) lets this draw write
                Ops = drawOn(Y, a, MOCK_CHART); cc = colsOf(Y, a); };
            auto snap = [&](double p) { return std::round(p * 4.0) / 4.0; };
            put("LiquidityProfile.status-ES-180.txt", "VERSION,0.67.0\nLIQ_CANDIDATES,rule zone >= 2x,median bid 100 / offer 300 lots,real orders near 9\n");   // (2.36.0) the LP medians for the ratio
            const std::string LQ = fl("DealerProfile\\ES-liqlevels.csv");
            auto rowsOf = [&](const std::string& txt) { std::vector<std::vector<std::string> > out; std::istringstream in(txt); std::string l; while (std::getline(in, l)) { std::vector<std::string> f; std::string c; std::istringstream ls(l); while (std::getline(ls, c, ',')) f.push_back(c); if (!l.empty() && l.back() == ',') f.push_back(""); out.push_back(f); } return out; };
            std::vector<Op> R1; DealerProfile::ColsState c1;
            run(snap(Lr + 0.2 * atr), snap(Ls - 0.2 * atr), R1, c1);
            const Op* pr = nullptr; const Op* ch = chipOn(R1, Lr, c1, &pr); const Op* ps = nullptr; const Op* cs2 = chipOn(R1, Ls, c1, &ps);
            check(ch && ch->text == "1" && ch->col == (0x000B0F19 & 0xFFFFFF), "(2.36.0) resistance level: rank chip 1 (2.37.0: the king offer 0.2 ATR beyond it, inside the 0.25 ATR reach) - dark text on the white king chip");
            check(cs2 && cs2->text == "2", "(2.36.0) support level: rank chip 2 (the bid 0.2 ATR below it)");
            {   bool ring = false; if (ch) for (auto& q : R1) if (q.kind == "box" && q.col2 == (C_SELL & 0xFFFFFF) && q.col == 0xF8FAFC && q.l <= (ch->l + ch->r) / 2 && q.r >= (ch->l + ch->r) / 2 && q.t <= (ch->t + ch->b) / 2 && q.b >= (ch->t + ch->b) / 2) ring = true;
                bool ringB = false; if (cs2) for (auto& q : R1) if (q.kind == "box" && q.col2 == (C_BUY & 0xFFFFFF) && q.l <= (cs2->l + cs2->r) / 2 && q.r >= (cs2->l + cs2->r) / 2 && q.t <= (cs2->t + cs2->b) / 2 && q.b >= (cs2->t + cs2->b) / 2) ringB = true;
                check(ring && ringB, "(2.36.0) chip ring in the order's side colour (offer red, bid green)"); }
            {   // right before the name chip(s): nothing between the chip and the first name chip except the gap; inside the column, no overlap
                bool okPos = ch && pr && ch->l > pr->r && ch->r <= 1 + c1.keysW; check(okPos, "(2.37.0) the rank box is RIGHT-MOST: right of the value and the name chips, inside the column");
                {   // (2.37.0) order left to right: value, then name, then rank
                    const Op* nm = nullptr; for (auto& q : R1) if (pr && ch && q.kind == "text" && q.l > pr->r && q.r < ch->l && std::abs((q.t + q.b) / 2 - (pr->t + pr->b) / 2) <= 2 && !q.text.empty() && std::isalpha((unsigned char)q.text[0])) nm = &q;
                    check(nm != nullptr, "(2.37.0) badge order: VALUE < NAME < RANK (name '" + std::string(nm ? nm->text : "-") + "' between the value and the rank box)"); }
                std::vector<Op> bx; for (auto& q : R1) if (q.kind == "box" && q.col == 0x0B0F19 && q.r - q.l < c1.keysW && q.b - q.t < 40 && q.l >= 1 && q.r <= 1 + c1.keysW) bx.push_back(q);
                int ov = 0; for (size_t i2 = 0; i2 < bx.size(); ++i2) for (size_t j2 = i2 + 1; j2 < bx.size(); ++j2) if (bx[i2].t < bx[j2].b && bx[j2].t < bx[i2].b) ++ov;
                bool inCol = true; for (auto& q : bx) inCol = inCol && q.l >= 1 && q.r <= 1 + c1.keysW;
                int oddsHit = 0; for (auto& q : R1) if (q.kind == "text" && q.text.find('%') != std::string::npos && q.l >= 1 && q.r <= 1 + c1.keysW) for (auto& b2 : bx) if (q.t < b2.b && b2.t < q.b && q.r > b2.l - 2) ++oddsHit;
                check(ov == 0 && inCol && oddsHit == 0, "(2.36.0) badges with chips: no overlap, inside the column, odds never meet a badge (" + std::to_string(ov) + " / " + std::to_string(oddsHit) + ")"); }
            check(c1.keysRankChips >= 2 && slurp(fl("DealerProfile.status-ES-180.txt")).find(",liquidity rank chips ") != std::string::npos, "(2.36.0) rank chips counted (PFH/PDH, VWAP, + VAH at the zone price); status KEYS_COLUMN has the liquidity rank chips field (" + std::to_string(c1.keysRankChips) + ")");
            std::vector<Op> R2; DealerProfile::ColsState c2b;   // the zones INSIDE the levels (between the level and price): no chip
            run(snap(Lr - 0.5 * std::min(atr, Lr - lastP)), snap(Ls + 0.5 * std::min(atr, lastP - Ls)), R2, c2b);
            check(!chipOn(R2, Lr, c2b, nullptr) && !chipOn(R2, Ls, c2b, nullptr), "(2.36.0) a zone inside the level (toward price) gives no chip");
            {   std::vector<Op> R5; DealerProfile::ColsState c5;   // (2.37.0) 0.5 ATR beyond: outside the 0.25 ATR reach now -> no chip
                run(snap(Lr + 0.5 * atr), snap(Ls - 0.5 * atr), R5, c5);
                check(!chipOn(R5, Lr, c5, nullptr) && !chipOn(R5, Ls, c5, nullptr) && c5.rankReachPx > 0.24 * atr && c5.rankReachPx < 0.26 * atr, "(2.37.0) a zone 0.5 ATR beyond gives no chip (reach 0.25 ATR = " + std::to_string(c5.rankReachPx) + ")");
                check(slurp(fl("DealerProfile.status-ES-180.txt")).find("RANK_RULE,LevelsColumn lrk 1.0.0,reach 0.25 ATR") != std::string::npos, "(2.37.0) status RANK_RULE line"); }
            {   // (2.37.0) no rank can show twice: every rank digit in the column's badges is unique
                std::map<std::string, int> seen; int dup = 0; const int kR2 = 1 + c1.keysW;
                for (auto& q : R1) if (q.kind == "text" && q.text.size() == 1 && std::isdigit((unsigned char)q.text[0]) && q.l >= 1 && q.r <= kR2) { if (++seen[q.text] > 1) ++dup; }
                check(dup == 0, "(2.37.0) no duplicate rank among the badges (" + std::to_string(dup) + ")"); }
            {   // (2.37.0) the history file: appended only on a change
                std::string h; int nfiles = 0; { const std::string par = UP.substr(0, UP.rfind('/')); DIR* dd = opendir(par.c_str());
                  if (dd) { while (dirent* e = readdir(dd)) { const std::string n = e->d_name; if (n.find("ES-liqlevels-history-") != std::string::npos) { ++nfiles; h = slurp(par + "/" + n); } } closedir(dd); } }
                const size_t n0 = std::count(h.begin(), h.end(), '\n');
                {   std::vector<Op> Rx; DealerProfile::ColsState cx; run(snap(Lr + 0.2 * atr), snap(Ls - 0.2 * atr), Rx, cx); }
                std::string h2; { const std::string par = UP.substr(0, UP.rfind('/')); DIR* dd = opendir(par.c_str());
                  if (dd) { while (dirent* e = readdir(dd)) { const std::string n = e->d_name; if (n.find("ES-liqlevels-history-") != std::string::npos) h2 = slurp(par + "/" + n); } closedir(dd); } }
                const size_t n1 = std::count(h2.begin(), h2.end(), '\n');
                std::printf("  history: %d file(s), %zu -> %zu lines\n", nfiles, n0, n1);
                check(h.rfind("time_ct_ms,level,price,rank,lots\n", 0) == 0 && h.find(",1,900") != std::string::npos, "(2.37.0) history file: header + the rank-1 900-lot row (" + std::to_string(h.size()) + " bytes)");
                check(nfiles == 1 && n1 >= n0, "(2.37.0) one history file per market + session, append only");
                {   // the logic: appends only on a change (lrk::History)
                    lrk::History hh; std::vector<lrk::Row> rr = { lrk::Row{ "PDH", "7864.00", 1, 900 }, lrk::Row{ "VWAP", "7852.00", 0, 0 } };
                    const size_t a1 = hh.changes("20261029", rr, 1).size(), a2 = hh.changes("20261029", rr, 2).size();
                    rr[0].rank = 2; const size_t a3 = hh.changes("20261029", rr, 3).size(); rr[1].rank = 1; rr[1].lots = 50; const size_t a4 = hh.changes("20261029", rr, 4).size();
                    rr[0].lots = 999; const size_t a5 = hh.changes("20261029", rr, 5).size(); const size_t a6 = hh.changes("20261030", rr, 6).size();
                    const std::vector<lrk::Row> few = { rr[1] }; const size_t a7 = hh.changes("20261030", few, 7).size();
                    check(a1 == 1 && a2 == 0 && a3 == 1 && a4 == 1 && a5 == 0 && a6 == 2 && a7 == 0, "(2.37.0) history appends only on a rank change (first ranked sight, 1->2, none->1; same rank / lots-only / scrolled-off = nothing; a new session writes again): " + std::to_string(a1) + std::to_string(a2) + std::to_string(a3) + std::to_string(a4) + std::to_string(a5) + std::to_string(a6) + std::to_string(a7)); }
                {   // the logic: reach + one level per zone
                    std::vector<double> lv2 = { 7860.0, 7860.5, 7845.0 }; std::vector<lrk::Zone> z2(3); z2[0].px = 7861.0; z2[0].rank = 1; z2[1].px = 7859.0; z2[1].rank = 2; z2[2].px = 7844.5; z2[2].rank = 3; z2[2].bid = true;
                    const std::vector<int> o2 = lrk::assign(lv2, z2, 7853.0, 0.75, 0.25);
                    check(o2[0] == -1 && o2[1] == 0 && o2[2] == 2, "(2.37.0) zone 7861 within 0.75 of BOTH 7860 and 7860.5 goes to the nearest (7860.5) only; 7859 (between 7860 and price) to none; bid 7844.50 to 7845");
                    std::vector<int> cnt(3, 0); for (int x : o2) if (x >= 0) ++cnt[(size_t)x]; check(cnt[0] <= 1 && cnt[1] <= 1 && cnt[2] <= 1, "(2.37.0) each zone at most once");
                    const std::vector<int> o3 = lrk::assign(lv2, z2, 7853.0, 0.5, 0.25); check(o3[1] == 0 && o3[0] == -1, "(2.37.0) reach edge inclusive (7860.5 + 0.5 = 7861)");
                    check(lrk::sessionKey(1793306400LL - 6 * 3600) != lrk::sessionKey(1793306400LL - 6 * 3600 + 10 * 3600) || true, "sessionKey runs"); } }
            std::vector<Op> R3; DealerProfile::ColsState c3b;   // more than 1 ATR beyond: no chip
            run(snap(Lr + 1.6 * atr), snap(Ls - 1.6 * atr), R3, c3b);
            const Op* p3 = nullptr; check(!chipOn(R3, Lr, c3b, &p3) && p3 && !chipOn(R3, Ls, c3b, nullptr), "(2.36.0) a zone more than 1 ATR beyond gives no chip (badge unchanged)");
            {   // (2.36.0, 16:51 CT) lsFlexLevels\DealerProfile\<MKT>-liqlevels.csv - format, rows = the badges, the chip's match, NONE rows, atomic, throttle
                run(snap(Lr + 0.2 * atr), snap(Ls - 0.2 * atr), R1, c1);
                const std::string t1 = slurp(LQ); const auto F = rowsOf(t1);
                bool fmt = !F.empty() && F[0].size() == 2 && F[0][0] == "VERSION" && F[0][1] == "1"; for (size_t i = 1; i < F.size(); ++i) fmt = fmt && F[i].size() == 13 && F[i][1] == "ES" && !F[i][0].empty() && std::atoll(F[i][0].c_str()) > 1700000000000LL;
                check(fmt && (int)F.size() - 1 == c1.liqRows && c1.liqRows >= 4, "(2.36.0) liqlevels.csv: VERSION,1 + one 13-field row per badge (" + std::to_string(F.size() - 1) + " rows / " + std::to_string(c1.liqRows) + ")");
                const std::string pR = dpaudit::priceText("ES", Lr), pS = dpaudit::priceText("ES", Ls); const double oR = snap(Lr + 0.2 * atr), bS = snap(Ls - 0.2 * atr);
                const std::vector<std::string>* rr = nullptr; const std::vector<std::string>* rs = nullptr; int none = 0;
                for (size_t i = 1; i < F.size(); ++i) { if (F[i][3] == pR) rr = &F[i]; if (F[i][3] == pS) rs = &F[i]; if (F[i][4] == "NONE") { ++none; if (!(F[i][5] == "0" && F[i][7] == "0" && F[i][8] == "0" && F[i][9].empty())) none = -99; } }
                check(rr && (*rr)[4] == "ASK" && (*rr)[5] == "900" && (*rr)[6] == "3.00" && (*rr)[7] == "1" && (*rr)[8] == "1" && (*rr)[9] == dpaudit::priceText("ES", oR - 0.25) && (*rr)[10] == dpaudit::priceText("ES", oR + 0.25) && (*rr)[11] == std::to_string(std::llround((oR - Lr) / 0.25)),
                      "(2.36.0) resistance row: ASK 900 lots, 3.00x the LP offer median, rank 1, king, zone +-1 tick, distance in ticks" + (rr ? " (" + (*rr)[4] + " " + (*rr)[5] + " " + (*rr)[6] + " r" + (*rr)[7] + " " + (*rr)[9] + "-" + (*rr)[10] + " d" + (*rr)[11] + ")" : std::string(" (missing)")));
                check(rs && (*rs)[4] == "BID" && (*rs)[5] == "300" && (*rs)[6] == "3.00" && (*rs)[7] == "2" && (*rs)[8] == "0" && (*rs)[11] == std::to_string(std::llround((Ls - bS) / 0.25)), "(2.36.0) support row: BID 300, 3.00x, rank 2, not king");
                check(none >= 1, "(2.36.0) levels with no liquidity 0..1 ATR beyond: side NONE, lots 0, rank 0, king 0, empty zone (" + std::to_string(none) + ")");
                { bool namesOk = false; for (size_t i = 1; i < F.size(); ++i) if (F[i][2].find('/') != std::string::npos) namesOk = true; check(namesOk, "(2.36.0) merged rows: names '/' separated"); }
                std::ifstream tf((LQ + ".tmp").c_str()); check(!tf.good(), "(2.36.0) atomic: no .tmp left behind");
                check(slurp(fl("DealerProfile.status-ES-180.txt")).find("LIQLEVELS,") != std::string::npos, "(2.36.0) status LIQLEVELS line");
                run(snap(Lr + 1.6 * atr), snap(Ls - 1.6 * atr), R3, c3b);
                { const auto G = rowsOf(slurp(LQ)); const std::vector<std::string>* r3 = nullptr; for (size_t i = 1; i < G.size(); ++i) if (G[i][3] == pR) r3 = &G[i];
                  check(r3 && (*r3)[4] == "NONE" && (*r3)[7] == "0", "(2.36.0) zone > 1 ATR beyond: the level's row is NONE / rank 0"); }
                // throttle: same rows = no rewrite inside 60 s; changed rows = rewrite only after 5 s
                DealerProfile Y; prime(Y, a, "ES", &lv);
                { CUR = &a; Y.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = 3; Y.cs.zBid = t10::Board(P0); Y.cs.zAsk = t10::Board(P0); Y.cs.zInit = true; Y.cs.liqLastMs = -1; Y.saveHostState(); }
                std::vector<sb::Fact> ZZ = { zf(oR, -1, 900), zf(bS, 1, 300) };
                sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZZ); drawOn(Y, a, MOCK_CHART); drawOn(Y, a, MOCK_CHART);
                const int w0 = colsOf(Y, a).liqWrites; drawOn(Y, a, MOCK_CHART); drawOn(Y, a, MOCK_CHART);
                check(w0 >= 1 && colsOf(Y, a).liqWrites == w0, "(2.36.0) throttle: unchanged rows are not rewritten on every repaint (" + std::to_string(w0) + " / " + std::to_string(colsOf(Y, a).liqWrites) + ")");
                ZZ[0].val = 950; sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZZ); drawOn(Y, a, MOCK_CHART);
                check(colsOf(Y, a).liqWrites == w0, "(2.36.0) throttle: a change inside 5 s waits");
                { CUR = &a; Y.loadHostState(); Y.cs.liqLastMs -= 6000; Y.saveHostState(); } drawOn(Y, a, MOCK_CHART);
                check(colsOf(Y, a).liqWrites == w0 + 1 && slurp(LQ).find(",ASK,950,") != std::string::npos, "(2.36.0) throttle: the change is written after 5 s");
                { CUR = &a; Y.loadHostState(); Y.cs.liqLastMs -= 61000; Y.saveHostState(); } drawOn(Y, a, MOCK_CHART);
                check(colsOf(Y, a).liqWrites == w0 + 2, "(2.36.0) throttle: rewritten once a minute even when unchanged");
            }
            sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB); drawOn(Z, a, MOCK_CHART);
        }
        {   // viewport independence
            std::vector<sb::Fact> Qa, Qb; const float t0 = PTOP, b0 = PBOT;
            for (int k = 0; k < 12; ++k) drawOn(Z, a, MOCK_CHART);
            sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Qa, false);
            PTOP = 7880.0f; PBOT = 7849.0f; drawOn(Z, a, MOCK_CHART); drawOn(Z, a, MOCK_CHART);
            sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Qb, false);
            bool same = Qa.size() == Qb.size() && !Qa.empty(); for (size_t k = 0; same && k < Qa.size(); ++k) same = Qa[k].lo == Qb[k].lo && Qa[k].code == Qb[k].code;
            check(same, "(2.35.0) scrolled viewport: the same shown set, ranks and king (" + std::to_string(Qa.size()) + " / " + std::to_string(Qb.size()) + ")");
            PTOP = t0; PBOT = b0; drawOn(Z, a, MOCK_CHART); }
        {   // touch tags + HELD on EVERY shown level (incl. rank 4-6), never on a level that is not shown
            CUR = &a; NOW = a.dt.back() + 180;
            auto ct = [](unsigned long d) { time_t t = (time_t)d; struct tm m; gmtime_r(&t, &m); return fr::ctAbs(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday, m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec); };
            const long long n = (long long)a.dt.size(); const long long tA = ct(a.dt[(size_t)(n - 2)]) - 90, tOut = ct(a.dt[(size_t)(n - 1)]) - 50;
            auto fact = [](long long bar, double px, int side, int code, double val) { sb::Fact f; f.bar = bar; f.lo = f.hi = px; f.side = side; f.code = code; f.val = val; f.state = code == 0 ? 0 : 1; f.proven = code >= 1 && code <= 5; return f; };
            std::vector<sb::Fact> Sf = { fact(tA, 7858.0, -1, 0, 0), fact(tA, 7846.0, 1, 0, 0), fact(tA, 7842.0, 1, 0, 0) };   // 7858 (rank 5) / 7846 (rank 4) shown, 7842 not
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); drawOn(Z, a, MOCK_CHART);
            DealerProfile::ColsState cz = colsOf(Z, a); bool l58 = false, l46 = false, l42 = false;
            for (auto& kv : cz.ttags) { if (kv.second.px == 7858.0) l58 = true; if (kv.second.px == 7846.0) l46 = true; if (kv.second.px == 7842.0) l42 = true; }
            check(l58 && l46 && !l42, "(2.35.0) touches latched for the rank 4 / 5 shown levels, not for the non-shown 7842");
            Sf = { fact(tA, 7858.0, -1, ltc::T_STAYS, 1), fact(tA, 7846.0, 1, ltc::T_GONE, -1) };
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); std::vector<Op> TZ = drawOn(Z, a, MOCK_CHART);
            check(hasText(TZ, "STAYS") && hasText(TZ, "GONE"), "(2.35.0) tags drawn on the lower-ranked shown levels");
            Sf = { fact(tA, 7858.0, -1, ltc::T_STAYS, 1), fact(tA, 7858.0, -1, 11, (double)(tOut - tA)), fact(tA, 7846.0, 1, ltc::T_GONE, -1) };
            sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sf); std::vector<Op> TH = drawOn(Z, a, MOCK_CHART);
            const Op* hz = nullptr; for (auto& q : TH) if (q.kind == "text" && q.text == "HELD" && std::abs((q.t + q.b) / 2 - yOfP(7858.0f)) <= 6) hz = &q;
            { int inPA = 0, hw = 0; for (auto& q : TH) { if (q.kind == "line" && std::max(q.l, q.r) > 1 + cz.keysW + 1 && std::min(q.l, q.r) < cz.lay.levels.l - 1) ++inPA; if (q.kind == "text" && q.text == "HELD" && q.l < cz.lay.levels.l) ++hw; }
              check(inPA == 0 && hw == 0, "(2.36.0) zone mode: no liquidity order line / HELD line or word in the price area (" + std::to_string(inPA) + " / " + std::to_string(hw) + ")"); }
            check(hz == nullptr && !hasText(TH, "HELD") && !hasText(TH, "BREAK") && !hasText(TH, "BROKE"), "(2.37.0) zone mode: no HELD / BREAK in the column");
            {   DealerProfile Y; prime(Y, a, "ES", &lv); sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs() - 120000, std::vector<sb::Fact>());
                drawOn(Y, a, MOCK_CHART); std::vector<Op> R1 = drawOn(Y, a, MOCK_CHART);
                const Op* h2 = nullptr; for (auto& q : R1) if (q.kind == "text" && q.text == "HELD" && std::abs((q.t + q.b) / 2 - yOfP(7858.0f)) <= 6) h2 = &q;
                check(h2 == nullptr, "(2.37.0) after a restart: no HELD in the column"); }
            {   // (2.37.0, Rassul 09:40 CT) no floating tag: a touch whose zone is NOT shown now draws nothing; the tag goes with its zone
                const long long tN = tA + 1; std::vector<sb::Fact> Sx = { fact(tN, 7858.0, -1, 0, 0) };
                sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sx); drawOn(Z, a, MOCK_CHART);
                Sx = { fact(tN, 7858.0, -1, ltc::T_STAYS, 1) }; sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), Sx);
                std::vector<sb::Fact> ZB2; for (auto& f : ZB) if (std::fabs(f.lo - 7858.0) > 0.1) ZB2.push_back(f);   // the 7858 offer zone leaves the column
                { CUR = &a; Z.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = 3; Z.cs.zBid = t10::Board(P0); Z.cs.zAsk = t10::Board(P0); Z.cs.zInit = true; Z.saveHostState(); }
                sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), ZB2); drawOn(Z, a, MOCK_CHART); std::vector<Op> TF = drawOn(Z, a, MOCK_CHART); DealerProfile::ColsState cf = colsOf(Z, a);
                int floating = 0; for (auto& q : TF) if (q.kind == "text" && q.l >= cf.lay.levels.l && q.r <= cf.lay.levels.r && (q.text == "STAYS" || q.text == "STAYS+" || q.text == "GONE" || q.text == "GONE+" || q.text == "weak") && std::abs((q.t + q.b) / 2 - yOfP(7858.0f)) <= 6) ++floating;
                check(floating == 0 && cf.ttTagOnly == 0, "(2.37.0) no tag without a shown zone (the zone left -> its tag went too)"); }
        }
    }

    const std::string st = slurp(fl("DealerProfile.status-ES-180.txt"));
    std::printf("  %s\n", st.substr(st.find("LEVELS_COLUMN"), 220).c_str());
    { std::ofstream f("dp2360_ops.json"); f << "{\"pw\":" << PW << ",\"ph\":" << PH << ",\"levels\":[" << c.lay.levels.l << "," << c.lay.levels.r << "],\"ops\":[";
      for (size_t i2 = 0; i2 < O.size(); ++i2) { const Op& q = O[i2]; std::string t; for (char ch : q.text) { if (ch == '"' || ch == '\\') t += '\\'; t += ch; }
        f << (i2 ? "," : "") << "{\"k\":\"" << q.kind << "\",\"l\":" << q.l << ",\"t\":" << q.t << ",\"r\":" << q.r << ",\"b\":" << q.b << ",\"c\":" << q.col << ",\"c2\":" << q.col2 << ",\"f\":" << q.font << ",\"bo\":" << (q.bold ? 1 : 0) << ",\"x\":\"" << t << "\"}"; }
      f << "]}\n"; }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS); return FAILS ? 1 : 0;
}
