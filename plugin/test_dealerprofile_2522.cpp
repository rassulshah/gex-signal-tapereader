// test_dealerprofile_2522.cpp -- lsDealerProfile 2.52.2 (2026-11-03 night, Rassul "fix all" after the Liquidity Profile audit: his ES Dealer
// Profile drew "K 7625.00 #2" from its own whole-book list while lsLiquidityProfile's king was offer 50 @ 7587; status "KING_NODES from
// lsLiquidityProfile B code 3: 0" - lsLiquidityProfile 0.70.0 had stopped posting code 3). Through the REAL DealerProfile.cpp on the mocked
// host of test_dealerprofile_2521.cpp (same whole-book scene). Checks:
//   A) an older lsLiquidityProfile (no code 9): unchanged - K = the biggest node (rank 1 of the board);
//   B) lsLiquidityProfile 0.73 READY with its king = bid 7845 (board rank 2): K is drawn on 7845, the 900 offer becomes "2", the rest keep
//      their order; Q facts rank 1 = 7845; status KING_NODES "K = the LP king";
//   C) the LP king is not on the board (7810, 300 lots): added as K, every node one rank lower;
//   D) LP ready with NO king: no "K" anywhere (rank 1 shows "1"), no "K" edge marker / chip;
//   E) LP WARMING (code 9 state 1): title "DP v2.52.2  LIQUIDITY \xB7 warming" inside the column, no rank text in any box, no rank chips on
//      the level badges, no edge-marker text, status LIQ_WARM warming;
//   F) END-TO-END: the B facts test_lp073_host.cpp captured from the real lsLiquidityProfile 0.73.0 (tests_liqtouch/lp073_bfacts.txt: its
//      king bid 7830 seeded from depth, a bigger 400 node at 7810 outside today's range) -> K = 7830, the 400 node is "2".
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2522.cpp -Wl,--unresolved-symbols=ignore-all -o t && ASAN_OPTIONS=detect_leaks=0 ./t
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
// (2521) IRT_LJ_INSET models what his chart shows (screenshot 2026-11-03): a LEFT-justified rect text lands ~4 px right of the rect's
// left edge (the rank digits 4-5 px right of the box centre, the price text 4 px right of x+1); centred text is centred. 0 = the old mock.
static int IRT_LJ_INSET = 0;
void cppExtension::RCT::drawText(const char* t, RTBOOL ctr, RTBOOL rj) {
    const int w = tw(t); const int l = ctr ? (left + right) / 2 - w / 2 : rj ? right - w : left + IRT_LJ_INSET;   // (2521) centred = in the rect's middle
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
// one LIQUIDITY row: its rank box, the rank text and everything else drawn on the row
struct Row { const Op* box = nullptr; const Op* rk = nullptr; std::vector<const Op*> rest; };
static std::vector<Row> rowsOf(const std::vector<Op>& O, int lqL, int lqR)
{
    std::vector<Row> R;
    for (const Op& q : O) if (q.kind == "text" && q.l >= lqL && q.r <= lqR + 2 && q.text.size() <= 2 && (std::isdigit((unsigned char)q.text[0]) || q.text == "K")) {
        Row r; r.rk = &q;
        for (const Op& b : O) if (b.kind == "box" && b.font == 1 && b.l >= lqL && b.r - b.l < 24 && b.l <= q.l && b.r >= q.r - 1 && b.t <= (q.t + q.b) / 2 && b.b >= (q.t + q.b) / 2) r.box = &b;
        if (!r.box) { R.push_back(r); continue; }
        const int yc = (r.box->t + r.box->b) / 2, ycT = (r.rk->t + r.rk->b) / 2;   // texts (textLJ rects) compare with the rank text, boxes with the rank box
        for (const Op& o : O) if (&o != r.box && &o != r.rk && (o.kind == "text" || o.kind == "box") && o.l >= lqL && o.r <= lqR + 40 && std::abs((o.t + o.b) / 2 - (o.kind == "text" ? ycT : yc)) <= 3 && o.r - o.l < 300) r.rest.push_back(&o);
        R.push_back(r);
    }
    return R;
}
static bool hit(const Op& a, const Op& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
static void checkRows(const std::vector<Op>& O, const DealerProfile::ColsState& c, int lqL, const std::string& tag, bool wantLetters, int wantRows = 8)
{
    const int lqR = c.lay.levels.r;
    const std::vector<Row> R = rowsOf(O, lqL, lqR);
    int noBox = 0, notFirst = 0, out = 0, ovl = 0, aligned = 0, style = 0, letters = 0, labelAfterBar = 0, rows = 0; int boxL = -1; std::string why;
    for (const Row& r : R) {
        if (!r.box) { ++noBox; continue; } ++rows;
        if (boxL < 0) boxL = r.box->l; if (r.box->l == boxL && boxL == lqL + 4) ++aligned;
        const bool king = r.rk->text == "K";
        if ((king ? r.box->col == 0xF8FAFC : r.box->col == 0x1E293B) && (r.box->col2 == 0x22C55E || r.box->col2 == 0xEF4444 || r.box->col2 != r.box->col) && r.rk->bold && (king ? r.rk->col != 0xF8FAFC : r.rk->col == 0xF8FAFC)) ++style;
        if (r.box->r > lqR || r.rk->r > lqR) ++out;
        const Op* bar = nullptr; std::vector<const Op*> texts;
        for (const Op* o : r.rest) {
            if (o->l <= r.box->r) { ++notFirst; if (why.empty()) why = o->kind + " '" + o->text + "' l " + std::to_string(o->l) + " <= rank box r " + std::to_string(r.box->r); }
            if (o->r > lqR) ++out;
            if (hit(*o, *r.box)) ++ovl;
            if (o->kind == "box" && (!bar || o->r - o->l > bar->r - bar->l)) bar = o;
            if (o->kind == "text") texts.push_back(o);
        }
        bool isL = false;
        for (const Op* t : texts) { isL = t->text == "R" || t->text == "S" || t->text == "\xACP" || t->text == "F"; if (isL) ++letters;
            else if (bar && t->l < bar->r) { ++ovl; if (why.empty()) why = "label '" + t->text + "' on the bar"; } else if (bar && t->l == bar->r + 4) ++labelAfterBar; }
        for (size_t i = 0; i < texts.size(); ++i) for (size_t j = i + 1; j < texts.size(); ++j) if (hit(*texts[i], *texts[j])) { ++ovl; if (why.empty()) why = "'" + texts[i]->text + "' / '" + texts[j]->text + "'"; }
    }
    check(rows == wantRows && noBox == 0, tag + ": " + std::to_string(wantRows) + " liquidity rows, each with its rank box (" + std::to_string(rows) + ", no box " + std::to_string(noBox) + ")");
    check(aligned == rows, tag + ": every rank box at the FRONT, one aligned column at the Liquidity column's left edge + 4 (" + std::to_string(aligned) + " / " + std::to_string(rows) + ", x " + std::to_string(boxL) + " vs " + std::to_string(lqL + 4) + ")");
    check(notFirst == 0, tag + ": the rank box is left of everything else on its row (" + std::to_string(notFirst) + (why.empty() ? "" : ": " + why) + ")");
    check(ovl == 0, tag + ": nothing on a row overlaps (" + std::to_string(ovl) + (why.empty() ? "" : ": " + why) + ")");
    check(out == 0, tag + ": every row inside the Liquidity column, right edge " + std::to_string(lqR) + " (" + std::to_string(out) + " out)");
    check(style == rows, tag + ": same box style / colours (K white box + dark letter, others dark box + white digit, side-colour border) (" + std::to_string(style) + " / " + std::to_string(rows) + ")");
    check(labelAfterBar >= rows && c.lqGapMax == 4, tag + ": the label still starts 4 px after its bar (" + std::to_string(labelAfterBar) + ", gap " + std::to_string(c.lqGapMax) + ")");
    if (wantLetters) check(letters == 4, tag + ": the king's R S notP F letters drawn, right of its rank box (" + std::to_string(letters) + ")");
    for (const Row& r : R) if (r.box) { std::string s = "  row [" + r.rk->text + "] box " + std::to_string(r.box->l) + "-" + std::to_string(r.box->r) + " |";
        std::vector<const Op*> v = r.rest; std::sort(v.begin(), v.end(), [](const Op* a, const Op* b) { return a->l < b->l; });
        for (const Op* o : v) s += " " + (o->kind == "box" ? std::string("bar") : "'" + o->text + "'") + "@" + std::to_string(o->l) + "-" + std::to_string(o->r);
        std::printf("%s\n", s.c_str()); }
}

static std::vector<sb::Fact> nodesB(double last)
{
    struct N { double px; long long lots; };
    const N nodes[] = { { 7858.00, 900 }, { 7845.00, 500 }, { 7870.00, 490 }, { 7820.00, 480 }, { 7900.00, 470 },
                        { 7865.75, 465 }, { 7833.00, 460 }, { 7841.00, 455 }, { 7880.00, 450 }, { 7837.00, 440 }, { 7836.00, 300 } };
    std::vector<sb::Fact> B;
    for (const N& n : nodes) { sb::Fact f; f.bar = 1; f.lo = f.hi = n.px; f.side = n.px < last ? 1 : -1; f.val = (double)n.lots; f.code = 4; f.proven = (int)n.lots; B.push_back(f); }
    return B;
}
static sb::Fact fact(int code, int side, double px, double val, int state, int proven) { sb::Fact f; f.bar = 1; f.lo = f.hi = px; f.side = side; f.val = val; f.code = code; f.state = state; f.proven = proven; return f; }
static void freshBoard(DealerProfile& X, Chart& a) { CUR = &a; X.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = DP_BOOKN; X.cs.bBoard = t10::Board(P0); X.cs.bInit = true; X.saveHostState(); }
// the rank text of the LIQUIDITY row at a price (by its y), "" = no row / blank box
static std::string rankAt(const std::vector<Op>& O, int lqL, int lqR, double px, bool* boxSeen = nullptr)
{
    const int y = yOfP((float)px); std::string r; if (boxSeen) *boxSeen = false;
    for (const Op& b : O) if (b.kind == "box" && b.font == 1 && b.l == lqL + 4 && b.r - b.l < 24 && std::abs((b.t + b.b) / 2 - y) <= 8) {
        if (boxSeen) *boxSeen = true;
        for (const Op& q : O) if (q.kind == "text" && q.l >= b.l && q.r <= b.r + 1 && q.t <= (b.t + b.b) / 2 && q.b >= (b.t + b.b) / 2) r = q.text; }
    return r;
}
static int rankTexts(const std::vector<Op>& O, int lqL, int lqR) { int n = 0; for (const Row& r : rowsOf(O, lqL, lqR)) if (r.box) ++n; return n; }
int main(int argc, char** argv)
{
    if (argc > 1) DUMP = argv[1];
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2522XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp"), set = slurp("DealerProfileSettings.h");
        check(std::string(DP_VERSION) == "2.52.2" && src.find("p->setVersion(\"2.52.2\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && set.find("PLUGIN_VERSION = \"2.52.2\"") != std::string::npos, "versions 2.52.2 agree (DP_VERSION, setVersion, settings PLUGIN_VERSION)");
    }
    dealerFile(true);
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    lv.push_back(klv::Merged{ "Open", 7850.0, false }); lv.push_back(klv::Merged{ "VAH", 7865.5, false }); lv.push_back(klv::Merged{ "DPoc", 7840.0, false }); lv.push_back(klv::Merged{ "VWAP", 7852.0, false });
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 150);
    drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 150);
    const double last = 7853.25;
    auto scene = [&](const std::vector<sb::Fact>& B) { sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), std::vector<sb::Fact>()); sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B);
        freshBoard(X, a); drawOn(X, a, MOCK_CHART); return drawOn(X, a, MOCK_CHART); };
    // ---- A) an older lsLiquidityProfile (no code 9): unchanged
    std::vector<Op> O = scene(nodesB(last)); DealerProfile::ColsState c = colsOf(X, a);
    const int lqL = c.lay.levels.l + c.keysW, lqR = c.lay.levels.r;
    check(rankAt(O, lqL, lqR, 7858.0) == "K" && rankAt(O, lqL, lqR, 7845.0) == "2", "A) no LP 0.73 state: K = the biggest node 7858 (unchanged), 7845 = 2");
    check(!c.lpProto && c.kOn && !c.rkHide, "A) state: no LP protocol, K on, ranks shown");
    // ---- B) LP 0.73 ready, its king = the 500 bid at 7845 (board rank 2)
    {   std::vector<sb::Fact> B = nodesB(last); B.push_back(fact(9, 0, 0, 900, 0, 64)); B.push_back(fact(3, 1, 7845.0, 500, 0, 520)); B.push_back(fact(3, -1, 7900.0, 470, 0, 400));
        O = scene(B); c = colsOf(X, a);
        check(rankAt(O, lqL, lqR, 7845.0) == "K", "B) K = the LP king 7845 (" + rankAt(O, lqL, lqR, 7845.0) + ")");
        check(rankAt(O, lqL, lqR, 7858.0) == "2" && rankAt(O, lqL, lqR, 7870.0) == "3" && rankAt(O, lqL, lqR, 7820.0) == "4", "B) the 900 offer is 2, then 3 / 4 in board order (" + rankAt(O, lqL, lqR, 7858.0) + rankAt(O, lqL, lqR, 7870.0) + rankAt(O, lqL, lqR, 7820.0) + ")");
        int ks = 0; for (const Op& q : O) if (q.kind == "text" && q.text == "K") ++ks;
        check(ks >= 1 && countPrefix(O, "K ") == 0, "B) exactly the LP king carries K (" + std::to_string(ks) + " K texts, chips included)");
        checkRows(O, c, lqL, "B) LP king", false, rankTexts(O, lqL, lqR));
        std::vector<sb::Fact> Q; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q, false);
        bool q1 = false; for (auto& f : Q) if (f.code == 1) q1 = std::fabs(f.lo - 7845.0) < 1e-6; check(q1, "B) Q facts: rank 1 = 7845 (the other indicators see the same king)");
        const std::string st = slurp(fl("DealerProfile.status-ES-180.txt"));
        check(st.find("K = the LP king 7845.00 500 lots (its node)") != std::string::npos && st.find("LP kings posted 2") != std::string::npos && st.find("LIQ_WARM,ready") != std::string::npos, "B) status KING_NODES / LIQ_WARM");
        {   size_t k = st.find("KING_NODES"); if (k != std::string::npos) std::printf("  %s\n", st.substr(k, st.find('\n', k) - k).c_str()); }
    }
    // ---- C) the LP king is not on the board: added as K
    {   std::vector<sb::Fact> B = nodesB(last); B.push_back(fact(9, 0, 0, 900, 0, 64)); B.push_back(fact(3, 1, 7810.0, 300, 0, 900));
        O = scene(B); c = colsOf(X, a);
        check(rankAt(O, lqL, lqR, 7810.0) == "K" && rankAt(O, lqL, lqR, 7858.0) == "2" && rankAt(O, lqL, lqR, 7845.0) == "3", "C) a king off the board is added as K; 7858 = 2, 7845 = 3 (" + rankAt(O, lqL, lqR, 7810.0) + "/" + rankAt(O, lqL, lqR, 7858.0) + "/" + rankAt(O, lqL, lqR, 7845.0) + ")");
        check(slurp(fl("DealerProfile.status-ES-180.txt")).find("K = the LP king 7810.00 300 lots (added)") != std::string::npos, "C) status: added");
    }
    // ---- D) LP ready, no king
    {   std::vector<sb::Fact> B = nodesB(last); B.push_back(fact(9, 0, 0, 900, 0, 64));
        O = scene(B); c = colsOf(X, a);
        int ks = 0; for (const Op& q : O) if (q.kind == "text" && (q.text == "K" || q.text.rfind("K ", 0) == 0 || q.text.rfind("~K", 0) == 0)) ++ks;
        check(ks == 0 && rankAt(O, lqL, lqR, 7858.0) == "1", "D) no LP king: no K anywhere, the biggest node shows 1 (" + rankAt(O, lqL, lqR, 7858.0) + ")");
        check(slurp(fl("DealerProfile.status-ES-180.txt")).find("K = none (no LP king)") != std::string::npos, "D) status: none (no LP king)");
    }
    // ---- E) LP WARMING (a stray code 3 is ignored)
    {   std::vector<sb::Fact> B = nodesB(last); B.push_back(fact(9, 0, 0, 240, 1, 5)); B.push_back(fact(3, 1, 7845.0, 500, 0, 520));
        O = scene(B); c = colsOf(X, a);
        const Op* title = nullptr; const Op* warmT = nullptr;
        for (const Op& q : O) if (q.kind == "text" && q.text.rfind("DP v2.52.2", 0) == 0) title = &q;
        for (const Op& q : O) if (q.kind == "text" && q.text.find("warming") != std::string::npos) warmT = &q;
        check(title && warmT && (warmT->text == "LIQUIDITY \xB7 warming" || title->text == "DP v2.52.2  LIQUIDITY \xB7 warming"), "E) the title says 'LIQUIDITY \xB7 warming' (" + (title ? title->text : std::string("none")) + " / " + (warmT ? warmT->text : std::string("none")) + ")");
        check(title && warmT && title->r <= lqR && warmT->r <= lqR && title->l >= lqL && warmT->l >= lqL, "E) title + warming inside the Liquidity column (" + (title ? std::to_string(title->l) + "-" + std::to_string(title->r) : std::string("-")) + ", " + (warmT ? std::to_string(warmT->l) + "-" + std::to_string(warmT->r) : std::string("-")) + " vs " + std::to_string(lqL) + "-" + std::to_string(lqR) + ")");
        {   int hit2 = 0; for (const Op& q : O) if (warmT && &q != warmT && &q != title && q.l < warmT->r && warmT->l < q.r && q.t < warmT->b && warmT->t < q.b && q.l >= lqL && q.r <= lqR && q.b - q.t < 60) { ++hit2; std::printf("  overlap %s '%s' %d-%d %d-%d vs %d-%d %d-%d\n", q.kind.c_str(), q.text.c_str(), q.l, q.r, q.t, q.b, warmT->l, warmT->r, warmT->t, warmT->b); }
            check(hit2 == 0, "E) the warming text overlaps nothing in the column (" + std::to_string(hit2) + ")"); }
        // the normal title (not warming) is unchanged
        int rk = 0; for (const Row& r : rowsOf(O, lqL, lqR)) if (r.box) ++rk;
        bool box = false; rankAt(O, lqL, lqR, 7858.0, &box);
        check(rk == 0 && box, "E) the rows keep their (empty) rank boxes, no rank / K text in any (" + std::to_string(rk) + ")");
        int chips = 0; for (const Op& q : O) if (q.kind == "text" && q.r <= lqL && q.l >= c.lay.levels.l && q.text.size() <= 2 && (q.text == "K" || std::isdigit((unsigned char)q.text[0]))) ++chips;
        check(chips == 0 && countPrefix(O, "#") == 0 && countPrefix(O, "K ") == 0, "E) no rank chips on the level badges, no edge-marker ranks (" + std::to_string(chips) + ")");
        const std::string st = slurp(fl("DealerProfile.status-ES-180.txt"));
        check(st.find("LIQ_WARM,warming,LP order book tracked 240 s") != std::string::npos && st.find("K = none (warming)") != std::string::npos, "E) status LIQ_WARM warming / K none");
        std::vector<sb::Fact> Q; sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q, false);
        check(!Q.empty(), "E) the Q facts (sizes) are still posted for the other indicators");
    }
    // ---- F) END-TO-END: the real lsLiquidityProfile 0.73.0's B facts (captured by tests_liqtouch/test_lp073_host.cpp)
    {   std::istringstream in(slurp("tests_liqtouch/lp073_bfacts.txt")); std::string ln; std::vector<sb::Fact> B; double kpx = 0, big = 0, bigPx = 0;
        while (std::getline(in, ln)) { if (ln.empty() || ln[0] == '#') continue; std::vector<std::string> v = dl::split(ln, ',');
            if (v.size() < 6) continue; sb::Fact f = fact(std::stoi(v[0]), std::stoi(v[1]), std::stod(v[2]), std::stod(v[3]), std::stoi(v[4]), std::stoi(v[5]));
            if (f.code == 3 && (kpx == 0 || f.val > big)) { if (f.val > big) { big = f.val; kpx = f.lo; } }
            if (f.code == 4 && f.val > bigPx) bigPx = f.val;
            B.push_back(f); }
        check(kpx == 7830.0 && B.size() >= 3, "F) the captured facts hold the LP king bid 7830 (" + std::to_string(B.size()) + " facts)");
        O = scene(B); c = colsOf(X, a);
        check(rankAt(O, lqL, lqR, 7830.0) == "K", "F) end-to-end: the Dealer Profile K row = lsLiquidityProfile's king 7830.00 (" + rankAt(O, lqL, lqR, 7830.0) + ")");
        check(rankAt(O, lqL, lqR, 7810.0) == "2", "F) the bigger 400 node outside today's range (its old K) is 2 (" + rankAt(O, lqL, lqR, 7810.0) + ")");
        check(c.kingMatched && !c.rkHide, "F) the king is its own node on the board; ready");
    }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
