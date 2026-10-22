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
    char tmpl[] = "/tmp/dp2190XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   // versions: the constant = the setVersion literal gex-build reads = the settings table; the shared headers
        const std::string src = slurp("DealerProfile.cpp");
        check(std::string(DP_VERSION) == "2.19.0" && src.find("p->setVersion(\"2.19.0\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == "2.19.0", "version 2.19.0 everywhere");
        check(std::string(cb::VERSION) == "1.3.0" && cb::range(cb::LEVELS).hi == 250 && std::string(lpct::VERSION) == "1.0.0", "ColumnBoard 1.3.0 (Levels up to 250), LevelPctLogic 1.0.0");
        DealerProfile E; E.setup();
        check(PX.levelPct >= 0 && PX.levelPct < (int)PARAMS.size() && PARAMS[PX.levelPct] == "Level % badges" && MOCK_CHK[PX.levelPct], "setting 'Level % badges', on by default");
    }
    dealerFile(true);
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    lv.push_back(klv::Merged{ "Open", 7850.0, false });                // today's RTH open: PFL/PDL below (LOD), PDH / PFH / PWH above (HOD)
    put("LRA-LevelBadges-ES.csv", badgeFile("ES", 11, 22));
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 80);
    drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 80);
    std::vector<Op> O = drawOn(X, a, MOCK_CHART);
    DealerProfile::ColsState c = colsOf(X, a);
    std::vector<Op> P; for (auto& q : O) if (q.kind == "text" && isPct(q.text)) P.push_back(q);
    bool hod = false, lod = false, fmt = true, q = false;
    for (auto& t : P) { if (t.text == "11%  /  40%") hod = true; if (t.text == "22%  /  41%") lod = true; if (t.text != "11%  /  40%" && t.text != "22%  /  41%") fmt = false; if (t.text.find('?') != std::string::npos) q = true; }
    check(P.size() >= 4 && hod && lod, "both sides on one chart: HOD numbers above the open, LOD below (" + std::to_string(P.size()) + " blocks)");
    check(fmt && !q, "exact '33%  /  66%' format, no '?'");
    for (auto& t : P) {   // the side follows the open (7850): the y of the text vs the open's y
        const int yc = (t.t + t.b) / 2; const bool above = yc < yOfP(7850.0f);
        check(above == (t.text == "11%  /  40%"), "side from the RTH open: " + t.text + " at y " + std::to_string(yc));
    }
    int out = 0; for (auto& t : P) if (t.l < c.lay.levels.l || t.r > c.lay.levels.r) ++out;
    check(out == 0, "every % block inside the Levels column (" + std::to_string(out) + " outside)");
    std::string why; check(strays(O, c, &why) == 0, "nothing in the Delta column or past the columns (" + why + ")");
    // the block boxes: the dark fill boxes that hold a % text; none overlaps another box on its row
    std::vector<Op> fills; for (auto& q2 : O) if (q2.kind == "box" && q2.l >= c.lay.levels.l && q2.r <= c.lay.levels.r) fills.push_back(q2);
    int ov = 0; for (size_t i = 0; i < fills.size(); ++i) for (size_t j = i + 1; j < fills.size(); ++j)
        if (fills[i].t < fills[j].b && fills[j].t < fills[i].b && fills[i].l < fills[j].r && fills[j].l < fills[i].r) ++ov;
    check(ov == 0, "no badge block overlaps another (" + std::to_string(ov) + ")");
    int green = 0, grey = 0; for (auto& q2 : O) if (q2.kind == "line" && q2.l >= c.lay.levels.l) { if (q2.col == lpct::C_GREEN) ++green; if (q2.col == lpct::C_GREY) ++grey; }
    check(green >= 4 && grey >= 4, "outlines: green (beats random) and grey (no better) both drawn (" + std::to_string(green) + " / " + std::to_string(grey) + ")");
    const std::string st = slurp(fl("DealerProfile.status.txt"));
    check(st.find("LEVEL_PCT,on,file ok,rows 22,built 2026-10-21,shown ") != std::string::npos && st.find("VERSION,2.19.0") != std::string::npos, "status LEVEL_PCT line");
    check(c.lay.levels.w() == 150 + DP_PCT_W, "Levels column 150 + the % block (" + std::to_string(c.lay.levels.w()) + ")");
    // the hover: on the % block only (every other hover stays off)
    {
        const Op& t = P[0];
        RTX_EVENT e; memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(t.l + 3); e.v.mouse.v = (short)((t.t + t.b) / 2); e.v.mouse.bar = -1; e.v.mouse.price = NAN;
        dp_test_cursor_wnd = nullptr; CUR = &a; X.mouse(&e);
        std::vector<Op> H = drawOn(X, a, MOCK_CHART);
        bool rnd = false, ex = false, use = false; for (auto& o : H) if (o.kind == "text") { if (o.text.find("random price") != std::string::npos) rnd = true; if (o.text.find("Example") != std::string::npos) ex = true; if (o.text.find("How to use") != std::string::npos) use = true; }
        check(rnd && ex && use, "hover on the % block: random comparison, how to use, example");
        e.v.mouse.h = (short)(c.lay.levels.l + 4); CUR = &a; X.mouse(&e);
        std::vector<Op> H2 = drawOn(X, a, MOCK_CHART);
        bool any = false; for (auto& o : H2) if (o.kind == "text" && (o.text.find("random price") != std::string::npos || o.text.find("Touch odds") != std::string::npos)) any = true;
        check(!any, "no hover on the name block (the 2.17.3 hovers-off stays)");
        e.v.mouse.h = 5; CUR = &a; X.mouse(&e); drawOn(X, a, MOCK_CHART);
    }
    // wrong market / garbled / missing: no block, the reason on the status line
    struct Case { std::string body; const char* reason; } cases[] = {
        { badgeFile("NQ", 11, 22), "MARKET NQ is not ES" }, { "VERSION,1\nMARKET,ES\nL,PDL,LOD,150,10\n", "10 fields" }, { "", "empty file" } };
    for (auto& k : cases) {
        put("LRA-LevelBadges-ES.csv", k.body); CUR = &a; X.loadHostState(); X.cs.lpChecked = -1; X.cs.lpStamp = -3; X.saveHostState();
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); int n = 0; for (auto& t : W) if (t.kind == "text" && isPct(t.text)) ++n;
        const std::string s2 = slurp(fl("DealerProfile.status.txt"));
        check(n == 0 && s2.find(k.reason) != std::string::npos, std::string("bad file -> no block + reason: ") + k.reason);
    }
    std::remove(fl("LRA-LevelBadges-ES.csv").c_str()); CUR = &a; X.loadHostState(); X.cs.lpChecked = -1; X.cs.lpStamp = -3; X.saveHostState();
    { std::vector<Op> W = drawOn(X, a, MOCK_CHART); int n = 0; for (auto& t : W) if (t.kind == "text" && isPct(t.text)) ++n;
      check(n == 0 && slurp(fl("DealerProfile.status.txt")).find("no file LRA-LevelBadges-ES.csv") != std::string::npos, "missing file -> no block + reason"); }
    // two charts on one DLL, two markets: each reads its own market's file, never the other's numbers
    {
        put("LRA-LevelBadges-ES.csv", badgeFile("ES", 11, 22)); put("LRA-LevelBadges-NQ.csv", badgeFile("NQ", 33, 44));
        CUR = &a; X.loadHostState(); X.cs.lpChecked = -1; X.cs.lpStamp = -3; X.saveHostState();
        Chart b = loadBars("14:57", "NQ", "NQZ6"); prime(X, b, "NQ", &lv);
        drawOn(X, b, "NQ 3m"); std::vector<Op> B = drawOn(X, b, "NQ 3m"); std::vector<Op> A = drawOn(X, a, MOCK_CHART);
        bool bNq = false, bEs = false, aEs = false, aNq = false;
        for (auto& t : B) if (t.kind == "text") { if (t.text.rfind("33%", 0) == 0 || t.text.rfind("44%", 0) == 0) bNq = true; if (t.text.rfind("11%", 0) == 0 || t.text.rfind("22%", 0) == 0) bEs = true; }
        for (auto& t : A) if (t.kind == "text") { if (t.text.rfind("11%", 0) == 0 || t.text.rfind("22%", 0) == 0) aEs = true; if (t.text.rfind("33%", 0) == 0 || t.text.rfind("44%", 0) == 0) aNq = true; }
        check(bNq && !bEs && aEs && !aNq, "two charts, two markets: each its own file");
        CUR = &b; X.done();
    }
    // the setting off: no block, the Levels column back to 150
    {
        const bool edited = true;
        put("DealerProfile.settings-ES.txt", std::string(ss::MAGIC) + "|DealerProfile|2.19.0|ES\nlevel_pct=0\n");
        CUR = &a; X.loadHostState(); X.saveHostState();
        std::vector<Op> W = drawOn(X, a, MOCK_CHART); W = drawOn(X, a, MOCK_CHART); int n = 0; for (auto& t : W) if (t.kind == "text" && isPct(t.text)) ++n;
        DealerProfile::ColsState c2 = colsOf(X, a);
        check(n == 0 && c2.lay.levels.w() == 150, "setting off: no block, Levels column 150 (" + std::to_string(n) + ", " + std::to_string(c2.lay.levels.w()) + (edited ? ", edited" : ", appended") + ")");
        if (n != 0) std::printf("  settings file: %s\n", slurp(fl("DealerProfile.settings-ES.txt")).c_str());
        X.done();
    }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
