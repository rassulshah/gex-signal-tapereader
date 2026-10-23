// test_dealerprofile_2200.cpp -- lsDealerProfile 2.20.0 (2026-10-22, Rassul approved the QUEUE METER 13:51 CT): the meter in the Levels column
// through the REAL DealerProfile.cpp on the mocked host (harness copied from test_dealerprofile_2190.cpp): a level badge becomes "PDH Q76%"
// with the fill, a tested price with no level gets a small unlabeled badge, the yellow originals-gone state, reach / stale / garbled /
// setting off = normal badges, colours from the settings, nothing outside the Levels column, the hover (? not proven yet), the status line.
// Build: g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2200.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
// (harness) test_dealerprofile_2190.cpp -- lsDealerProfile 2.19.0 (2026-10-21, Rassul approved Option B; 22:52 "show both hod and lod badges"):
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
#undef private
#undef protected
#include <cstdio>
#include <sys/stat.h>
#include <utime.h>

static int FAILS = 0, PASSES = 0;
static void check(bool c, const std::string& m) { if (c) ++PASSES; else { ++FAILS; std::printf("FAIL %s\n", m.c_str()); } }

// ------------------------------------------------------------------------------------------------ the mocked host (records draws)
struct Chart { std::string root, sym; int spb = 180; void* user = nullptr;
               std::vector<unsigned long> dt; std::vector<float> o, h, l, c; std::vector<unsigned long> v; };
static Chart* CUR = nullptr;
static const float PTOP = 7935.0f, PBOT = 7795.0f; static const int PH = 820;
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
static std::string qmFile(const std::string& mkt, double atr, const std::vector<std::string>& rows)
{
    std::string s = std::string("VERSION,1.0.0\nMARKET,") + mkt + "\nASOF,1791536100000\nTICK,0.25\nLAST,7853.25\nATR," + std::to_string(atr) + "\n";
    for (auto& r : rows) s += r + "\n";
    return s;
}
static const std::string T_PDH = "T,A,7863.25,7861.75,7866.75,L,PDH,120,91,76,40,5.0,9.0,1.0,4.0,-1,L,refilling,0,1,-1,1791536000000,0,1";   // (2.21.0) call RED, ES = SHOW
static const std::string T_PIV = "T,B,7849.00,7845.50,7849.50,P,,63,21,33,0,5.5,15.9,14.5,0.0,1.0,H,pulled,1,1,-1,1791536050000,-1,1";   // call pending
static const std::string T_FAR = "T,B,7834.50,7831.00,7835.00,L,PFL,50,40,80,50,1.0,9.0,0.0,0.0,-1,L,eaten,0,1,-1,1791536060000";   // a 1.0.0 row (22 fields): still read, no call
static void reloadQm(DealerProfile& X, Chart& a) { CUR = &a; X.loadHostState(); X.cs.qmChecked = -1; X.cs.qmStamp = -3; X.saveHostState(); }
static std::vector<Op> meterTexts(const std::vector<Op>& O) {
    std::vector<Op> r;
    for (auto& q : O) {
        if (q.kind != "text") continue;
        if ((q.text.find("  Q ") != std::string::npos || q.text.rfind("Q ", 0) == 0) && q.text.find_first_of("0123456789") != std::string::npos) r.push_back(q);   // (2.23.0) 'PDH  Q 91' / 'Q 21 (0.3x)': lots, no percent
        else if (q.text.rfind("Q", 0) == 0 && q.text.find('%') != std::string::npos) r.push_back(q);
    }
    return r;
}
int main()
{
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/home/claude/qmwork/dp2200XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp");
        check(src.find(std::string("p->setVersion(\"") + DP_VERSION + "\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && std::string(DP_VERSION) == "2.23.0", "version 2.23.0: DP_VERSION = setVersion literal = settings table");
        const ss::Spec& sp = dprset::spec();
        check(sp.paramVersion == 9 && std::string(sp.rows[dprset::K_QUEUE_METER].key) == "queue_meter" && std::string(sp.rows[dprset::K_QM_REACH].key) == "qm_reach"
              && sp.rows[dprset::K_QM_BID].type == ss::T_COLOR && sp.rows[dprset::K_QM_WARN].type == ss::T_COLOR && sp.rows[dprset::K_QM_REACH].type == ss::T_FLOAT, "settings: Queue meter checkbox, Reach (ATR) float, 3 colours; parameter version 9");
        check(sp.rows[dprset::K_QM_REACH].lo == 0.25 && sp.rows[dprset::K_QM_REACH].hi == 2.0 && sp.rows[dprset::K_QM_REACH].def == 1.0, "reach 0.25 - 2.0 ATR, default 1.0");
    }
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    lv.push_back(klv::Merged{ "Open", 7850.0, false });
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 80);
    // 0) no file: normal badges, the reason on the status line
    std::vector<Op> base = drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 80); base = drawOn(X, a, MOCK_CHART);
    check(meterTexts(base).empty() && slurp(fl("DealerProfile.status.txt")).find("QUEUE_METER,on,reach 1 ATR,file no file LRA-QueueLive-ES.csv") != std::string::npos, "no file: no meter, reason in the status");
    // 1) a live file: PDH offers 76% (a level badge becomes the meter), a pivot 33% with originals gone (unlabeled, yellow), PFL beyond the reach
    put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { T_PDH, T_PIV, T_FAR })); reloadQm(X, a);
    std::vector<Op> O = drawOn(X, a, MOCK_CHART);
    DealerProfile::ColsState c = colsOf(X, a);
    std::vector<Op> M = meterTexts(O);
    const Op* pdh = nullptr; const Op* piv = nullptr; bool far = false;
    for (auto& t : M) { if (t.text.find("Q 91") != std::string::npos) pdh = &t; if (t.text.rfind("Q 21", 0) == 0) piv = &t; if (t.text.find("Q 40") != std::string::npos) far = true; }
    check(pdh && pdh->text.rfind("PD", 0) == 0 && pdh->text.find("  Q 91") != std::string::npos, "the PDH badge reads 'PDH  Q 91' (" + std::string(pdh ? pdh->text : "none") + ")");
    check(pdh && pdh->col == 0xFFFFFF, "normal state: white text");
    check(piv && piv->text == "Q 21 . orig 0%" && piv->col == 0xFACC15, "a pivot with no level: unlabeled 'Q33% . orig 0%' in yellow (" + std::string(piv ? piv->text : "none") + ")");
    check(!far, "a test beyond the reach (PFL 18.75 pts away, reach 1 x 12) keeps its normal badge");
    int out = 0; for (auto& t : M) if (t.l < c.lay.levels.l || t.r > c.lay.levels.r) ++out;
    check(out == 0, "meter text inside the Levels column (" + std::to_string(out) + " outside)");
    std::string why; check(strays(O, c, &why) == 0, "nothing in the Delta / Dealer columns or past them (" + why + ")");
    if (pdh) {   // the fill: a box from the badge's left edge, ~76% of the badge; the outline red (offers)
        const int yc = (pdh->t + pdh->b) / 2; int bgW = 0, fillW = 0; bool redLine = false;
        for (auto& q : O) {
            if (q.kind == "box" && q.t <= yc && q.b >= yc && q.l == c.lay.levels.l + 2) { if (q.col == 0x191919) bgW = q.r - q.l; else fillW = std::max(fillW, q.r - q.l); }
            if (q.kind == "line" && q.col == 0xEF4444 && std::min(q.t, q.b) <= yc + 12 && std::max(q.t, q.b) >= yc - 12 && std::min(q.l, q.r) >= c.lay.levels.l) redLine = true;
        }
        check(bgW > 0 && std::abs(fillW - (int)std::lround(bgW * 0.76)) <= 1, "fill = 76% of the badge (" + std::to_string(fillW) + " of " + std::to_string(bgW) + ")");
        check(bgW == c.lay.levels.w() - 3, "the meter spans the Levels column, never past it (" + std::to_string(bgW) + " / " + std::to_string(c.lay.levels.w()) + ")");
        check(redLine, "the 10-s call RED: red outline");
    }
    {   std::vector<Op> fills; for (auto& q2 : O) if (q2.kind == "box" && q2.l >= c.lay.levels.l && q2.r <= c.lay.levels.r && q2.col == 0x191919) fills.push_back(q2);
        int ov = 0; for (size_t i = 0; i < fills.size(); ++i) for (size_t j = i + 1; j < fills.size(); ++j)
            if (fills[i].t < fills[j].b && fills[j].t < fills[i].b && fills[i].l < fills[j].r && fills[j].l < fills[i].r) ++ov;
        check(ov == 0, "no badge overlaps another (" + std::to_string(ov) + ")"); }
    {   int yl = 0; for (auto& q : O) if (q.kind == "line" && q.col == 0xFACC15 && q.font == 2) ++yl;
        check(yl >= 4, "originals gone: 2 px yellow outline (" + std::to_string(yl) + ")"); }
    const std::string st = slurp(fl("DealerProfile.status.txt"));
    check(st.find("QUEUE_METER,on,reach 1 ATR,file ok,live tests 3") != std::string::npos && st.find("on a level badge 1,unlabeled 1,beyond reach 1") != std::string::npos, "status QUEUE_METER line");
    if (st.find("on a level badge 1,unlabeled 1,beyond reach 1") == std::string::npos) { size_t p = st.find("QUEUE_METER"); if (p != std::string::npos) std::printf("  %s\n", st.substr(p, 220).c_str()); }
    // 2) the hover on the meter: ? not proven yet, the numbers, question / what / why / how / example
    if (pdh) {
        RTX_EVENT e; memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(pdh->l + 3); e.v.mouse.v = (short)((pdh->t + pdh->b) / 2); e.v.mouse.bar = -1; e.v.mouse.price = NAN;
        dp_test_cursor_wnd = nullptr; CUR = &a; X.mouse(&e);
        std::vector<Op> H = drawOn(X, a, MOCK_CHART);
        bool q = false, ql = false, tte = false, risk = false, ex = false, use = false;
        for (auto& o : H) if (o.kind == "text") {
            const std::string& t = o.text;
            if (t.find("Queue meter ?") != std::string::npos) q = true;   // ES is SHOW (2.21.0): no '?' 
            if (t.find("Queue left: 91 of 120") != std::string::npos) ql = true;
            if (t.find("Time to empty") != std::string::npos) tte = true;
            if (t.find("Break risk: LOW") != std::string::npos) risk = true;
            if (t.find("Example") != std::string::npos) ex = true;
            if (t.find("How to use") != std::string::npos) use = true;
        }
        check(ql && tte && risk && ex && use && !q, "hover (ES = SHOW: no '?' in the header), queue left, time to empty, break risk, how to use, example");
        e.v.mouse.h = 5; CUR = &a; X.mouse(&e); drawOn(X, a, MOCK_CHART);
    }
    // 3) stale (the Liquidity Profile stopped) / wrong market / torn row: normal badges
    {   struct utimbuf u; u.actime = u.modtime = time(nullptr) - 30; utime(fl("LRA-QueueLive-ES.csv").c_str(), &u); reloadQm(X, a);
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); check(meterTexts(W).empty(), "a stale file (30 s old): normal badges"); }
    {   put("LRA-QueueLive-ES.csv", qmFile("NQ", 12.0, { T_PDH })); reloadQm(X, a);
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); check(meterTexts(W).empty() && slurp(fl("DealerProfile.status.txt")).find("file wrong market") != std::string::npos, "wrong market: normal badges + reason"); }
    {   put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { T_PDH.substr(0, 40) })); reloadQm(X, a);
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); check(meterTexts(W).empty() && slurp(fl("DealerProfile.status.txt")).find("garbled T row") != std::string::npos, "torn row: normal badges + reason"); }
    // 4) the reach setting is real (2.0 ATR brings PFL in); the colour setting is used; the checkbox removes the meter
    {   put("DealerProfile.settings-ES.txt", std::string(ss::MAGIC) + "|DealerProfile|2.20.0|ES\nqm_reach=2\nqm_bid=0000FF\n");
        put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { T_PDH, T_FAR })); reloadQm(X, a);
        CUR = &a; X.loadHostState(); X.saveHostState();
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); W = drawOn(X, a, MOCK_CHART);
        bool farOn = false; for (auto& t : meterTexts(W)) if (t.text.find("Q 40") != std::string::npos) farOn = true;
        check(farOn, "reach 2 ATR: the PFL test (18.75 pts, 1.6 ATR) becomes a meter");
        bool blue = false; const unsigned long want = (unsigned long)mix((COLOR)0x0000FF, 0.45f) & 0xFFFFFF; for (auto& q : W) if (q.kind == "box" && q.col == want) blue = true;
        check(blue, "the bid colour setting is used (the PFL meter's fill is blue)");
        if (!blue) { for (auto& q : W) if (q.kind == "box" && q.l > 900) std::printf("  box %d-%d %d-%d col %06lx\n", q.l, q.r, q.t, q.b, q.col); }
        bool grey = false; for (auto& q : W) if (q.kind == "line" && q.col == (C_MUTED & 0xFFFFFF)) grey = true;
        check(grey, "no call (a 1.0.0 row): 1 px grey outline");
        put("DealerProfile.settings-ES.txt", std::string(ss::MAGIC) + "|DealerProfile|2.20.0|ES\nqueue_meter=0\n");
        CUR = &a; X.loadHostState(); X.saveHostState();
        W = drawOn(X, a, MOCK_CHART); W = drawOn(X, a, MOCK_CHART);
        check(meterTexts(W).empty() && slurp(fl("DealerProfile.status.txt")).find("QUEUE_METER,off (setting)") != std::string::npos, "checkbox off: no meter, normal badges");
        int nb = 0; for (auto& q : W) if (q.kind == "text" && q.text.rfind("PD", 0) == 0) ++nb;
        check(nb >= 1, "the normal PDH badge is back");
    }
    // 4b) the call in a SHOW? market: "?" after the text; the hover gives the market's odds line
    {   put("DealerProfile.settings-ES.txt", std::string(ss::MAGIC) + "|DealerProfile|2.20.0|ES\n");
        std::string q = T_PDH; q.replace(q.size() - 1, 1, "2");   // mark SHOW?
        put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { q })); reloadQm(X, a);
        CUR = &a; X.loadHostState(); X.saveHostState();
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); W = drawOn(X, a, MOCK_CHART);
        bool qm2 = false; for (auto& t : meterTexts(W)) if (t.text.find("Q 91") != std::string::npos && t.text.find("?") == std::string::npos) qm2 = true;
        check(qm2, "SHOW? market: 'PDH  Q 91' - no '?' on the badge (1.2.0, Rassul 17:55)");
        put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { T_PDH })); reloadQm(X, a);
        W = drawOn(X, a, MOCK_CHART); const std::vector<Op> MT = meterTexts(W); const Op* t0 = nullptr; for (auto& t : MT) if (t.text.find("Q 91") != std::string::npos) t0 = &t;
        if (t0) { RTX_EVENT e; memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(t0->l + 3); e.v.mouse.v = (short)((t0->t + t0->b) / 2); e.v.mouse.bar = -1; e.v.mouse.price = NAN;
            CUR = &a; X.mouse(&e); std::vector<Op> H = drawOn(X, a, MOCK_CHART); bool odds = false, rec = true;
            for (auto& o : H) if (o.kind == "text" && o.text.find("broke 76% of the time (base 30%)") != std::string::npos) odds = true;
            check(odds, "hover: ES odds 'queue left under 40% at 10 s broke 76% of the time (base 30%)'");
            e.v.mouse.h = 5; CUR = &a; X.mouse(&e); drawOn(X, a, MOCK_CHART); (void)rec; }
        else check(false, "the PDH meter for the hover");
    }
    // 5) a narrow Levels column: the text shrinks / shortens and still never leaves the column
    {   put("DealerProfile.settings-ES.txt", std::string(ss::MAGIC) + "|DealerProfile|2.20.0|ES\nlevels_w=30\nlevel_pct=0\n");
        put("LRA-QueueLive-ES.csv", qmFile("ES", 12.0, { T_PDH, T_PIV })); reloadQm(X, a);
        CUR = &a; X.loadHostState(); X.saveHostState();
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); W = drawOn(X, a, MOCK_CHART);
        DealerProfile::ColsState c2 = colsOf(X, a); int o2 = 0, n2 = 0; for (auto& t : meterTexts(W)) { ++n2; if (t.l < c2.lay.levels.l || t.r > c2.lay.levels.r) ++o2; }
        std::string w2; check(n2 >= 1 && o2 == 0 && strays(W, c2, &w2) == 0, "narrow column: meters fit inside (" + std::to_string(n2) + " drawn, " + std::to_string(o2) + " outside " + w2 + ")");
        X.done();
    }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
