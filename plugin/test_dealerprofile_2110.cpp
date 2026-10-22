// test_dealerprofile_2110.cpp -- (CR5200) lsDealerProfile 2.11.0 on a mocked host (the real irtsdk.h, every drawing call recorded,
// the real 13 Oct ES files in tests_dp290/): the right-side columns (Levels | Delta | Dealer), the right margin, the one-sided teal /
// orange nodes with B / S text, the compact level badges with liquidity (design B), the settings window, markets, narrow charts.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2110.cpp -Wl,--unresolved-symbols=ignore-all
// (preamble from test_dealerprofile_290.cpp: the real irtsdk.h, the drawing calls recorded)
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
// (2026-10-22 test refresh) the approved face since this suite was written: the node text is "9.0K lots" (DP 2.16.0, Rassul 2026-10-16
// 22:16 "just give the number followed by lots" - colour says buy / sell, no "B " / "S "); the Levels column is at least 150 px
// (2.14.0 124 px "NAME | price | liquidity", 2.14.1 150 px for NQ prices) plus the 92 px level % block when "Level % badges" is on
// (2.19.0, ColumnBoard 1.3.0 range 30-250) - DealerProfile.cpp lvlW()
static bool isNodeText(const std::string& t) { return t.size() > 5 && t.compare(t.size() - 5, 5, " lots") == 0; }
static int countNodeText(const std::vector<Op>& O) { int n = 0; for (auto& q : O) if (q.kind == "text" && isNodeText(q.text)) ++n; return n; }
static int LV(int setting, bool pct = true) { return std::min(250, std::max(setting, 150) + (pct ? 92 : 0)); }
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
int main(int argc, char** argv)
{
    if (argc > 1) DUMP = argv[1];
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2110XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   // ---- setup(): controls only, the agreed rows and defaults (Dealer 80 / Levels 78 / Badge 9 / Liquidity in badge / Show B / S text)
        DealerProfile E; E.setup();
        bool rows = PARAMS.size() == 14 &&   /* (2.17.2) 11 rows, 3 hidden; (2.19.0) + "Level % badges"; (2.20.0) + the 5 queue meter rows */ PARAMS[PX.width] == "Dealer column px (40-250)" && PARAMS[PX.levelsW] == "Levels column px (30-150)" && PARAMS[PX.badgeFont] == "Badge font (pt)"
                    && PARAMS[PX.liqBadge] == "Liquidity in badge" && PARAMS[PX.labels] == "Show buy / sell text in node";
        check(rows && MOCK_INT[PX.width] == 80 && MOCK_INT[PX.levelsW] == 78 && MOCK_INT[PX.badgeFont] == 9 && MOCK_CHK[PX.liqBadge] && MOCK_CHK[PX.labels], "setup: the column rows with their defaults (80 / 78 / 9, both boxes on)");
        if (!rows) for (size_t i = 0; i < PARAMS.size(); ++i) std::printf("  param %zu %s\n", i, PARAMS[i].c_str());
        bool howto = false; for (auto& r : PARAMS) if (r.find("How") != std::string::npos || r.size() > 60) howto = true;
        check(!howto, "setup: controls only (no how-to text)");
    }
    // ---- 0. versions: DP_VERSION = the setVersion literal gex-build reads = the header
    {
        const std::string src = slurp("DealerProfile.cpp");
        // (2026-10-22) the literal moves with every approved build (2.12.0 -> 2.19.0 so far): DP_VERSION must equal the setVersion literal
        check(src.find(std::string("p->setVersion(\"") + DP_VERSION + "\")") != std::string::npos, std::string("version ") + DP_VERSION + ": constant = setVersion literal");
        check(std::string(cb::VERSION) >= "1.0.0" && std::string(dcol::VERSION) >= "1.1.0", std::string("ColumnBoard ") + cb::VERSION + " / DealerColumnsLogic " + dcol::VERSION + " (at least 1.0.0 / 1.1.0)");
    }
    // ---- 1. the default layout on ES: Delta Profile on the chart (board), Levels 78 | Delta 80 | Dealer 80, the margin
    {
        dealerFile(true);
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES");
        MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 80);
        drawOn(X, a, MOCK_CHART);                                              // first draw: peers seen
        cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 80);
        std::vector<Op> O = drawOn(X, a, MOCK_CHART);
        DealerProfile::ColsState c = colsOf(X, a);
        const int lv0 = LV(78), lvL = 1440 - lv0;   // (2.19.0) 150 + 92 = 242 px: Levels 1198-1440
        check(c.lay.ok && c.lay.dealer.l == 1520 && c.lay.dealer.r == 1600 && c.lay.delta.l == 1440 && c.lay.levels.l == lvL && c.lay.priceRight == lvL - 20, "default: Levels " + std::to_string(lvL) + "-1440 | Delta 1440-1520 | Dealer 1520-1600, price ends at " + std::to_string(lvL - 20));
        std::string why; const int s = strays(O, c, &why);
        check(s == 0, "nothing drawn into the Delta column or past the columns (" + std::to_string(s) + " " + why + ")");
        int nodes = 0, past = 0, teal = 0, orange = 0;
        for (auto& q : O) if (q.kind == "box" && q.r == 1598 && q.t > 40) { ++nodes; if (q.l < 1520) ++past; if (q.col == dcol::C_POS) ++teal; if (q.col == dcol::C_NEG) ++orange; }
        check(nodes > 5 && past == 0 && teal > 0 && orange > 0, "nodes one-sided at the Dealer column's right edge, teal + / orange -, never past its left edge");
        int bs = 0, bad = 0; for (auto& q : O) if (q.kind == "text" && isNodeText(q.text)) { ++bs; if (!inBox(q, c.lay.dealer) || q.col != (C_DARK & 0xFFFFFF)) ++bad; }
        check(bs > 0 && bad == 0, "(2.16.0) 'N lots' hedge text inside the nodes (dark ink), " + std::to_string(bs) + " shown");
        check(hasText(O, "CR0") && hasText(O, "HVL") && hasText(O, "PS0"), "compact MenthorQ badges CR0 / HVL / PS0");
        // (14:37 CT review) no W / F letters anywhere; no hourly swing names; every node text inside a node that is tall and long enough;
        // every badge text inside its block with its padding
        int wf = 0, hr = 0; for (auto& q : O) if (q.kind == "text") { if (q.text == "W" || q.text == "F") ++wf; if (q.text.find("HrHI") != std::string::npos || q.text.find("HrLO") != std::string::npos) ++hr; }
        check(wf == 0 && hr == 0, "no W / F letters and no HrHI / HrLO on the chart (" + std::to_string(wf) + " / " + std::to_string(hr) + ")");
        int badNode = 0, nodeTxt = 0;
        for (size_t i = 0; i < O.size(); ++i) if (O[i].kind == "text" && isNodeText(O[i].text)) {
            ++nodeTxt; const int yc = O[i].t + (O[i].b - O[i].t) / 2 + (int)std::lround(O[i].font * 0.45);   // the y the plugin asked for
            bool in = false; for (size_t j = 0; j < i; ++j) if (O[j].kind == "box" && O[j].r == 1598 && O[j].l <= O[i].l - 3 && O[j].r >= O[i].r + 3 && O[j].t <= yc - dcol::textPx(O[i].font) / 2 && O[j].b >= yc + dcol::textPx(O[i].font) / 2) in = true;
            if (!in) ++badNode; }
        check(nodeTxt > 0 && badNode == 0, "node text inside its node: 3 px each side, the full text height (" + std::to_string(badNode) + " of " + std::to_string(nodeTxt) + " not)");
        int badBadge = 0, badgeTxt = 0;
        for (size_t i = 0; i < O.size(); ++i) if (O[i].kind == "text" && O[i].l >= c.lay.levels.l && O[i].r <= c.lay.levels.r + 40 && O[i].font <= 9) {
            ++badgeTxt; const int yc = O[i].t + (O[i].b - O[i].t) / 2 + (int)std::lround(O[i].font * 0.45);
            bool in = false; for (size_t j = 0; j < i; ++j) if (O[j].kind == "box" && O[j].l <= O[i].l - 2 && O[j].r >= O[i].r + 2 && O[j].r <= c.lay.levels.r && O[j].t <= yc - dcol::textPx(O[i].font) / 2 - 2 && O[j].b >= yc + dcol::textPx(O[i].font) / 2 + 2) in = true;
            if (!in) ++badBadge; }
        check(badgeTxt > 3 && badBadge == 0, "badge text inside its block with padding, vertically centred (" + std::to_string(badBadge) + " of " + std::to_string(badgeTxt) + " not)");
        { const std::string st0 = slurp(fl("DealerProfile.status.txt")); check(st0.find("tiny nodes not drawn ") != std::string::npos && st0.find("tiny nodes not drawn 0") == std::string::npos, "strikes under 3% of the biggest wall: not drawn (counted)"); }
        int wide = 0; for (auto& q : O) if (q.kind == "text" && (q.text.find('%') != std::string::npos || q.text.find("STALE 24m") != std::string::npos || q.text.find("Liq ") != std::string::npos)) ++wide;
        check(wide == 0, "no wide level labels on the chart (odds / STALE / Liq are in the hovers; the price is in the badge since 2.14.0)");
        int cnt = 0; check(badgeOverlaps(O, c.lay.levels, &cnt) == 0 && cnt > 3, "badges inside the Levels column, none overlapping (" + std::to_string(cnt) + ")");
        // the margin: set in calc (not in draw), in bars: ceil(258 / 5) = 52; again = no new call
        check(mockOpt().sets == 0, "draw never sets the margin");
        CUR = &a; X.calc(0);
        const int need0 = 20 + LV(78) + 80 + 80;   // gap + Levels + Delta + Dealer (2.19.0: 422 px -> 85 bars at 5 px / bar)
        check(mockOpt().margin == (need0 + 4) / 5 && mockOpt().sets == 1, "calc: right margin " + std::to_string((need0 + 4) / 5) + " bars (" + std::to_string(need0) + " px at 5 px / bar), set once (" + std::to_string(mockOpt().margin) + ")");
        X.calc(0); X.preCalc();
        check(mockOpt().sets == 1, "the chart already has it: not set again");
        const std::string st = slurp(fl("DealerProfile.status.txt"));
        check(st.find("COLUMNS," + std::to_string(1440 - LV(78)) + "-1440,1440-1520,1520-1600,price right " + std::to_string(1420 - LV(78)) + ",clean,delta on") != std::string::npos && st.find("MARGIN,need " + std::to_string(need0) + " px,units bars,ppb 5") != std::string::npos && st.find(std::string("VERSION,") + DP_VERSION) != std::string::npos, "status: COLUMNS / MARGIN / VERSION lines");
        check(st.find("REACH,80") != std::string::npos, "REACH = the Dealer column (the Read / Analyst keep left of it)");
        // a hover on a badge: the detail moved off the chart
        RTX_EVENT e; memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE;
        { e.v.mouse.v = yOfP(7925.0f); } e.v.mouse.h = 1380; e.v.mouse.bar = -1; e.v.mouse.price = 7925.0f; dp_test_cursor_wnd = nullptr;
        CUR = &a; X.mouse(&e);
        std::vector<Op> H = drawOn(X, a, MOCK_CHART);
        bool q = false, od = false; for (auto& o : H) if (o.kind == "text") { if (o.text.find("Will ES react at CR0") == 0) q = true; if (o.text.find("Touch odds") == 0 || o.text.find("Data: STALE") == 0) od = true; }
        check(!q && !od, "(2.17.3, Rassul 2026-10-18 23:37 \"get rid of the hover over on the gex levels\") no badge hover drawn");
        X.done(); cb::clear(cb::DELTA, a.sym, a.spb, MOCK_CHART);
        int w = 0; long long ms = 0; check(!cb::peek(cb::DEALER, a.sym, a.spb, MOCK_CHART, w, ms) && !cb::peek(cb::LEVELS, a.sym, a.spb, MOCK_CHART, w, ms), "done(): the chart's columns leave the board");
    }
    // ---- 2. min / max / mixed widths (his settings per market) x Delta widths: boxes, node length, margin
    {
        const int G[] = { 40, 250, 120, 80 }, LW[] = { 30, 150, 78, 60 }, DW[] = { 40, 250, 80, 130 };
        int bad = 0; std::string why;
        for (int i = 0; i < 4; ++i) {
            settingsRow("ES", G[i], LW[i], 1);
            Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES");
            const std::string L = "ES widths " + std::to_string(i); MOCK_CHART = L; mockOpt() = MockOpts();
            cb::publish(cb::DELTA, a.sym, a.spb, L, 1, DW[i]); drawOn(X, a, L); cb::publish(cb::DELTA, a.sym, a.spb, L, 2, DW[i]);
            std::vector<Op> O = drawOn(X, a, L);
            DealerProfile::ColsState c = colsOf(X, a);
            const int sum = 20 + LV(LW[i]) + DW[i] + G[i];
            if (c.lay.w[cb::DEALER] != G[i] || c.lay.w[cb::LEVELS] != LV(LW[i]) || c.lay.w[cb::DELTA] != DW[i] || c.lay.margin != sum || !cb::columnsClean(c.lay)) { ++bad; why += " layout" + std::to_string(i); }
            if (strays(O, c, &why)) { ++bad; why += " stray" + std::to_string(i); }
            int maxL = 0; for (auto& q : O) if (q.kind == "box" && q.r == 1598 && q.t > 40) maxL = std::max(maxL, q.r - q.l);
            if (maxL != G[i] - 6) { ++bad; why += " node" + std::to_string(i) + "=" + std::to_string(maxL); }
            CUR = &a; X.calc(0);
            if (mockOpt().margin != (sum + 4) / 5) { ++bad; why += " margin" + std::to_string(i); }
            if (badgeOverlaps(O, c.lay.levels)) { ++bad; why += " overlap" + std::to_string(i); }
            X.done(); cb::clear(cb::DELTA, a.sym, a.spb, L);
        }
        check(bad == 0, "widths 40/30/40, 250/150/250, 120/78/80, 80/60/130: layout, no strays, full node = column - 6, margin, no overlap" + why);
        put("DealerProfile.settings.txt", "");
    }
    // ---- 3. a narrow chart: squeezed into the room (price keeps 160 px), and a chart with no room: nothing in the columns, no margin
    {
        settingsRow("ES", 250, 150, 1);
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES");
        PW = 700; MOCK_CHART = "ES narrow"; mockOpt() = MockOpts();
        cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 250); drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 250);
        std::vector<Op> O = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c = colsOf(X, a);
        std::string why;
        check(c.lay.squeezed && c.lay.ok && c.lay.priceRight >= 159 && cb::columnsClean(c.lay) && strays(O, c, &why) == 0, "700 px chart, 150 / 250 / 250 asked: squeezed, clean, price keeps 160 px " + why);
        X.done(); cb::clear(cb::DELTA, a.sym, a.spb, MOCK_CHART);
        Chart b = loadBars("14:57"); DealerProfile Y; prime(Y, b, "ES"); PW = 120; MOCK_CHART = "ES tiny"; mockOpt() = MockOpts();
        std::vector<Op> T = drawOn(Y, b, MOCK_CHART); DealerProfile::ColsState d = colsOf(Y, b);
        int inCols = 0; for (auto& q : T) if (q.kind != "line" && q.r > d.lay.priceRight) ++inCols;
        CUR = &b; Y.calc(0);
        check(!d.lay.ok && inCols == 0 && mockOpt().sets == 0, "120 px chart: no room for price -> nothing drawn in the columns, margin untouched");
        Y.done(); PW = 1600; put("DealerProfile.settings.txt", "");
    }
    // ---- 4. many close levels (30 within 7 points) + missing dealer data: stacked, no overlap, ticks, 'no data' in the column only
    {
        std::remove(fl("LRA-Dealer-ES.csv").c_str());
        std::vector<klv::Merged> many; const char* nm[] = { "PDH", "PDL", "ONH", "ONL", "PFH", "PFL", "LonHI", "LonLO", "AsiaHI", "AsiaLO", "PWH", "PWL", "CWH", "CWL" };
        for (int i = 0; i < 30; ++i) many.push_back(klv::Merged{ std::string(nm[i % 14]) + (i >= 14 ? std::to_string(i) : ""), 7856.0 + 0.25 * i, false });
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &many); MOCK_CHART = "ES many";
        std::vector<Op> O = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c = colsOf(X, a);
        int cnt = 0; const int ov = badgeOverlaps(O, c.lay.levels, &cnt);
        int ticks = 0; for (auto& q : O) if (q.kind == "line" && q.l == c.lay.levels.l && q.r == c.lay.levels.l + 1) ++ticks;
        int outside = 0; for (auto& q : O) if (q.kind == "box" && q.l == c.lay.levels.l + 2 && (q.t < 0 || q.b > PH)) ++outside;
        check(cnt >= 30 && ov == 0 && outside == 0, "30 close key levels (+ MenthorQ): " + std::to_string(cnt) + " badges, none overlapping, all inside the pane");
        check(ticks >= 10, "stacked badges: a tick from each line (" + std::to_string(ticks) + ")");
        check(hasText(O, "no data") && !hasText(O, "nan") && !hasText(O, "0") && countNodeText(O) == 0, "no dealer file: 'no data' in the column header, no nodes, no garbage");
        std::string why; check(strays(O, c, &why) == 0, "no dealer data: still nothing outside the columns " + why);
        CUR = &a; X.calc(0); check(mockOpt().margin > 0, "no dealer data: the margin is still kept for the columns");
        X.done(); dealerFile(true);
    }
    // ---- 5. market switch on one DLL object: ES / NQ / CL charts keep their own widths, board entries and layouts
    {
        settingsRow("ES", 80, 78, 1); settingsRow("NQ", 120, 100, 1); settingsRow("CL", 60, 40, 0);
        Chart es = loadBars("14:57", "ES", "ESZ6"), nq = loadBars("14:57", "NQ", "NQZ6", 17000.0f), cl = loadBars("14:57", "CL", "CLX6", -7800.0f);
        DealerProfile X; prime(X, es, "ES"); prime(X, nq, "NQ"); prime(X, cl, "CL");
        drawOn(X, es, "ES 3m"); drawOn(X, nq, "NQ 3m"); drawOn(X, cl, "CL 3m");
        std::vector<Op> E2 = drawOn(X, es, "ES 3m");
        DealerProfile::ColsState a = colsOf(X, es), b = colsOf(X, nq), c = colsOf(X, cl);
        int w = 0; long long ms = 0;
        check(a.lay.w[cb::DEALER] == 80 && a.lay.w[cb::LEVELS] == LV(78) && b.lay.w[cb::DEALER] == 120 && b.lay.w[cb::LEVELS] == LV(100) && c.lay.w[cb::DEALER] == 60 && c.lay.w[cb::LEVELS] == LV(40),
              "ES 80/78, NQ 120/100, CL 60/40: each chart its own market's widths (Levels >= 150 + the % block since 2.14.1 / 2.19.0)");
        check(cb::peek(cb::DEALER, "NQZ6", 180, "NQ 3m", w, ms) && w == 120 && cb::peek(cb::DEALER, "CLX6", 180, "CL 3m", w, ms) && w == 60 && cb::peek(cb::LEVELS, "ESZ6", 180, "ES 3m", w, ms) && w == LV(78), "board entries per chart");
        CUR = &es; X.loadHostState(); const bool esLiq = X.cfg.liqBadge; CUR = &cl; X.loadHostState(); const bool clLiq = X.cfg.liqBadge;
        check(esLiq && !clLiq, "settings never leak between markets (Liquidity in badge on ES, off on CL)");
        CUR = &es; X.done(); CUR = &nq; X.done(); CUR = &cl; X.done(); put("DealerProfile.settings.txt", "");
    }
    // ---- 6. level badges with liquidity (Rassul 11:39 CT, design B)
    {
        // CR0 7925 above price (~7860): 900 offers at 2.7x -> "HL 900" red, white outline, 3 px line; HVL 7850 below: 1,234 bids 2.6x ->
        // "HL 1.2K" green; PS0 7825 below: 310 bids 1.1x -> "310" grey on dark grey; G1 7900: no row -> the name only
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES"); MOCK_CHART = "ES liq";
        liqFile("L,31700,R,900,2.7\nL,31400,S,1234,2.6\nL,31300,S,310,1.1\n");
        cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 80); drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 80);
        std::vector<Op> O = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c = colsOf(X, a);
        if (DUMP) dump(O, a, c);
        auto boxAt = [&](const std::vector<Op>& V, const std::string& txt, unsigned long& fill) { for (size_t i = 0; i < V.size(); ++i) if (V[i].kind == "text" && V[i].text == txt) {
                for (size_t j = i; j-- > 0;) if (V[j].kind == "box" && V[j].l <= V[i].l && V[j].r >= V[i].r && V[j].t <= V[i].b && V[j].b >= V[i].t) { fill = V[j].col; return true; } } return false; };
        unsigned long f1 = 0, f2 = 0, f3 = 0;
        // (2.16.0, Rassul 2026-10-16 22:18) the liquidity block shows the RATIO ("2.7x", yellow), not "HL? 900" / "310"; the HL colour + outline stay
        check(boxAt(O, "2.7x", f1) && f1 == (C_SELL & 0xFFFFFF), "CR0 above price: the HL block '2.7x' on RED (resting offers)");
        check(boxAt(O, "2.6x", f2) && f2 == (C_BUY & 0xFFFFFF), "HVL below price: the HL block '2.6x' on GREEN (resting bids)");
        check(boxAt(O, "1.1x", f3) && f3 == 0x282828, "PS0 normal liquidity: '1.1x' on dark grey (40,40,40)");
        int outl = 0; for (auto& q : O) if (q.kind == "line" && q.col == 0xFFFFFF && q.font == 2 && q.l >= c.lay.levels.l) ++outl;
        check(outl >= 8, "HL blocks: a 2 px white outline (" + std::to_string(outl) + " segments)");
        int thick = 0, thin = 0; { const int y1 = yOfP(7925.0f), y2 = yOfP(7825.0f);
          for (auto& q : O) if (q.kind == "line" && q.l == 0 && q.r == PW) { if (q.t == y1 && q.font == 3) ++thick; if (q.t == y2 && q.font == 3) ++thin; } }
        check(thick == 0 && thin == 0, "(2.13.0, Rassul 2026-10-15 09:53) level lines only in the Levels column: no line across the chart, HL or not");
        int cnt = 0; check(badgeOverlaps(O, c.lay.levels, &cnt) == 0 && strays(O, c) == 0, "wider badges: still stacked inside the 78 px column, no overlap");
        int liqBlocks = 0; for (auto& q : O) if (q.kind == "box" && (q.col == 0x282828 || q.col == (C_BUY & 0xFFFFFF) || q.col == (C_SELL & 0xFFFFFF)) && q.l > c.lay.levels.l + 2 && q.r <= c.lay.levels.r) ++liqBlocks;
        check(liqBlocks == 3, "only the levels with liquidity get a block (" + std::to_string(liqBlocks) + "), the rest name only");
        // hysteresis: CR0 drops to 2.3x -> still HL (off only under 2.1x after 30 s)
        liqFile("L,31700,R,950,2.3\nL,31400,S,1234,2.6\nL,31300,S,310,1.1\n"); reloadLiq(X, a);
        std::vector<Op> O2 = drawOn(X, a, MOCK_CHART); unsigned long f4 = 0;
        check(boxAt(O2, "2.3x", f4) && f4 == (C_SELL & 0xFFFFFF), "2.5x -> 2.3x: stays HL (no flicker)");
        // price crosses the level: the Liquidity Profile now reports CR0's band as BIDS (below price) -> green
        liqFile("L,31700,S,980,2.8\n"); reloadLiq(X, a);
        std::vector<Op> O3 = drawOn(X, a, MOCK_CHART); unsigned long f5 = 0;
        check(boxAt(O3, "2.8x", f5) && f5 == (C_BUY & 0xFFFFFF), "price crossed CR0: the block flips to GREEN (bids)");
        // a stale file, then garbage, then 'Liquidity in badge' off: name-only, never 0 / garbage
        liqFile("L,31700,S,980,2.8\n", 30); reloadLiq(X, a);
        std::vector<Op> O4 = drawOn(X, a, MOCK_CHART);
        check(countPrefix(O4, "HL") == 0 && !hasText(O4, "2.8x") && !hasText(O4, "1.1x") && hasText(O4, "CR0"), "a file 30 s old: no liquidity in the badges");
        put("LRA-LiqLevels-ES.csv", "garbage\nL,31700,S,0,9\n"); reloadLiq(X, a);
        std::vector<Op> O5 = drawOn(X, a, MOCK_CHART);
        check(countPrefix(O5, "HL") == 0 && !hasText(O5, "0") && !hasText(O5, "9.0x") && !hasText(O5, "0.0x"), "a garbage file: no liquidity, no '0'");
        settingsRow("ES", 80, 78, 0); liqFile("L,31700,R,900,2.7\n");
        Chart b = loadBars("14:57"); DealerProfile Y; prime(Y, b, "ES");
        std::vector<Op> O6 = drawOn(Y, b, "ES liq off");
        check(countPrefix(O6, "HL") == 0 && !hasText(O6, "2.7x") && hasText(O6, "CR0"), "'Liquidity in badge' off: no liquidity in the badges");
        // a narrow Levels column: no room for the block -> the name alone (never cut)
        settingsRow("ES", 80, 30, 1); Chart n = loadBars("14:57"); DealerProfile Z; prime(Z, n, "ES");
        std::vector<Op> O7 = drawOn(Z, n, "ES liq narrow");
        int lvN = 0; { DealerProfile::ColsState cn = colsOf(Z, n); lvN = cn.lay.w[cb::LEVELS]; }
        check(countPrefix(O7, "HL") == 0 && lvN == LV(30), "(2.14.1) a 30 px Levels setting: the column stays " + std::to_string(LV(30)) + " px (150 + the % block), never narrower (" + std::to_string(lvN) + ")");
        CUR = &a; X.done(); CUR = &b; Y.done(); CUR = &n; Z.done(); put("DealerProfile.settings.txt", ""); cb::clear(cb::DELTA, a.sym, a.spb, "ES liq");
    }
    // ---- 7. the settings window: filled from the file (opening never saves); an untouched control keeps the file's value; garbage refused
    {
        settingsRow("ES", 120, 60, 1, 11);
        const std::string before = slurp(fl("DealerProfile.settings.txt"));
        Chart a = loadBars("14:57"); DealerProfile X; CUR = &a; X.loadHostState(); X.saveHostState();
        PERIOD = "";                                                   // the settings window's copy has no bar size
        memset(MOCK_INT, 0, sizeof(MOCK_INT));
        X.parmsLoad();
        check(slurp(fl("DealerProfile.settings.txt")) == before, "opening the window never writes the file");
        check(MOCK_INT[PX.width] == 120 && MOCK_INT[PX.levelsW] == 60 && MOCK_INT[PX.badgeFont] == 9 && MOCK_INT[PX.font] == 11 && MOCK_CHK[PX.liqBadge] && MOCK_CHK[PX.labels], "the window opens filled from the market's row (120 / 60 / 9 / 11, checked)");
        if (PX.market >= 0) MOCK_LIST_GET[PX.market] = MOCK_LIST[PX.market];   // (2.17.2) Market is hidden (no control)
        settingsRow("ES", 130, 60, 1, 11);                           // another window saved the Dealer width meanwhile
        MOCK_INT[PX.levelsW] = 90;                                   // he changes only the Levels column
        X.parmsApply();
        // (2.17.1) the values live in the keyed per-market file
        std::string row = keyedES();
        auto has = [&](const char* kv) { return row.find(std::string("\n") + kv + "\n") != std::string::npos; };
        check(has("width=130") && has("levels_w=90") && has("font=11") && has("badge_font=9") && has("liq_badge=1") && has("inds=cob_bull,cob_bear"), "Apply: Levels 90 saved, the untouched Dealer width keeps the file's 130 (" + row + ")");
        MOCK_INT[PX.width] = 9999; MOCK_INT[PX.badgeFont] = 99; X.parmsApply(); row = keyedES();
        check(has("width=130") && has("levels_w=90") && has("badge_font=9"), "garbage widths / fonts from the window never reach the file (" + row + ")");
        MOCK_CHK[PX.liqBadge] = false; X.parmsApply(); row = keyedES();
        check(has("liq_badge=0"), "the 'Liquidity in badge' box saved");
        PERIOD = "3 Minutes";
        // an old 2.10.0 row: the bar length 50 -> the 80 px Dealer column, the Levels default 78
        put("DealerProfile.settings.txt", "ES|0|50|10|0|1|1|1|0|cob_bull,cob_bear\nNQ|0|300|10|0|1|1|1|0|x\n");
        DealerProfile::State* s = static_cast<DealerProfile::State*>(a.user); (void)s;
        X.savedStamp = -3; X.syncSettings();
        check(X.cfg.width == 80 && X.cfg.levelsW == 78 && X.cfg.liqBadge, "old row (2.10.0, bar 50): Dealer column 80, Levels 78, liquidity on");
        X.done(); put("DealerProfile.settings.txt", "");
    }
    // ---- 8. an hourly chart (15-min+ bars): the Levels column only (no Dealer column), margin = gap + Levels
    {
        Chart a = loadBars("14:57"); for (size_t i = 0; i < a.dt.size(); ++i) a.dt[i] = a.dt[0] + (unsigned long)i * 3600UL;
        DealerProfile X; prime(X, a, "ES"); MOCK_CHART = "ES 1h"; mockOpt() = MockOpts();
        std::vector<Op> O = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c = colsOf(X, a);
        int w = 0; long long ms = 0;
        check(c.lay.dealer.w() == 0 && c.lay.levels.r == 1600 && c.lay.margin == 20 + LV(78) && !cb::peek(cb::DEALER, a.sym, a.spb, MOCK_CHART, w, ms), "hourly: Levels at the axis, no Dealer column, margin 20 + Levels");
        check(countNodeText(O) == 0 && hasText(O, "CR0"), "hourly: badges only, no nodes");
        X.done();
    }
    // ---- 9. the right-margin unit: IRT in pixels -> the probe switches and the margin is asked in pixels
    {
        dealerFile(true);
        Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES"); MOCK_CHART = "ES px"; mockOpt() = MockOpts();
        // the mocked last bar ends at x = 200 + 5 * (n - 1) + 2 (kBarRight ~ centre + half); make IRT 'report' a margin equal to that gap in px
        drawOn(X, a, MOCK_CHART); DealerProfile::ColsState c = colsOf(X, a);
        mockOpt().margin = c.gapSeen; drawOn(X, a, MOCK_CHART); drawOn(X, a, MOCK_CHART);
        DealerProfile::ColsState d = colsOf(X, a);
        CUR = &a; X.calc(0);
        check(d.probe.units == cb::PIXELS && mockOpt().margin == d.needPx, "a chart whose margin value equals the gap in px: pixels detected, margin asked in px (" + std::to_string(mockOpt().margin) + ")");
        mockOpt().fail = true; const int sets = mockOpt().sets; X.calc(0);
        check(mockOpt().sets == sets, "chart options unavailable: nothing set, nothing guessed");
        mockOpt().fail = false; X.done();
    }
    std::string rm = std::string("rm -rf ") + dir; if (std::system(rm.c_str()) != 0) {}
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
