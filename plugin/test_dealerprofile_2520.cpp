// test_dealerprofile_2520.cpp -- lsDealerProfile 2.52.0 (2026-11-03, Rassul "the rank in front of the liquidity", approved mockup rank_front.png)
// through the REAL DealerProfile.cpp on the mocked host of test_dealerprofile_2400.cpp (same whole-book scene: 8 rows, the king graded 4/4
// with R S notP F on its bar, rank 2 testing 1:49). Checks: the rank box is the FIRST thing of every LIQUIDITY row (one aligned column at
// the column's left edge + 4, same box style / colours: border = side colour, K white box + dark letter, others dark box + white digit),
// left of the letters, the bar and the "lots  score  pct%" label; nothing in a row overlaps (rank box vs bar / letters / label, label vs
// bar, label pieces vs each other); every row inside the Liquidity column (the box never grows off the chart); label still 4 px after the
// bar; versions 2.52.0 agree (DP_VERSION, setVersion, settings PLUGIN_VERSION).
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2520.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
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
static void checkRows(const std::vector<Op>& O, const DealerProfile::ColsState& c, int lqL, const std::string& tag, bool wantLetters)
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
    check(rows == 8 && noBox == 0, tag + ": 8 liquidity rows, each with its rank box (" + std::to_string(rows) + ", no box " + std::to_string(noBox) + ")");
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
int main(int argc, char** argv)
{
    if (argc > 1) DUMP = argv[1];
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2520XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    { std::string d = UP; mkdir(d.c_str(), 0700); }
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    {   const std::string src = slurp("DealerProfile.cpp"), set = slurp("DealerProfileSettings.h");
        check(std::string(DP_VERSION) == "2.52.0" && src.find("p->setVersion(\"2.52.0\")") != std::string::npos && std::string(dprset::PLUGIN_VERSION) == std::string(DP_VERSION) && set.find("PLUGIN_VERSION = \"2.52.0\"") != std::string::npos, "versions 2.52.0 agree (DP_VERSION, setVersion, settings PLUGIN_VERSION)");
        check(DP_LIQ_RANK_FRONT && !DP_LIQ_RANK_RIGHT && DP_BOOK && DP_RANK_CHIPS, "(2.52.0) the liquidity rank at the FRONT of the row (not the right end); level-badge rank chips unchanged");
    }
    dealerFile(true);
    std::vector<klv::Merged> lv;
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") lv.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    lv.push_back(klv::Merged{ "Open", 7850.0, false }); lv.push_back(klv::Merged{ "VAH", 7865.5, false }); lv.push_back(klv::Merged{ "DPoc", 7840.0, false }); lv.push_back(klv::Merged{ "VWAP", 7852.0, false });
    put("LRA-LevelBadges-ES.csv", badgeFile("ES", 11, 22));   // the odds file exists: the badges must still show NO odds
    Chart a = loadBars("14:57"); DealerProfile X; prime(X, a, "ES", &lv);
    MOCK_CHART = "ES 3m #1"; cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 1, 150);
    cb::publishKeys(a.sym, a.spb, MOCK_CHART, dpNowMs(), 230);   // a stale left Key Levels column from 2.39 on the board: 2.40 clears it
    drawOn(X, a, MOCK_CHART); cb::publish(cb::DELTA, a.sym, a.spb, MOCK_CHART, 2, 150);
    // the whole book (B code 4): price 7853.25, 3-min ATR 2.77
    struct N { double px; long long lots; };
    const N nodes[] = { { 7858.00, 900 }, { 7845.00, 500 }, { 7870.00, 490 }, { 7820.00, 480 }, { 7900.00, 470 },
                        { 7865.75, 465 },   // r6: 0.25 above VAH, 1.75 above PFH, 2.5 above PDH -> shown
                        { 7833.00, 460 },   // r7: 1.5 below PFL/PDL 7834.5 -> shown
                        { 7841.00, 455 },   // r8: 1.0 above DPoc 7840 (a POC, either side) -> shown
                        { 7880.00, 450 },   // r9: 50% but no level within 1 ATR -> not shown
                        { 7837.00, 440 },   // r10: 3.0 below DPoc (> 1 ATR), above the low level PFL -> not shown
                        { 7836.00, 300 } }; // r11: 33% -> not shown
    std::vector<sb::Fact> B;
    for (const N& n : nodes) { sb::Fact f; f.bar = 1; f.lo = f.hi = n.px; f.side = n.px < 7853.25 ? 1 : -1; f.val = (double)n.lots; f.code = 4; f.proven = (int)n.lots; B.push_back(f); }
    { sb::Fact s2; s2.bar = 1; s2.lo = s2.hi = 7854.0; s2.side = -1; s2.val = 5000; s2.code = 2; B.push_back(s2); }   // an old code-2 zone: ignored in book mode
    sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B);
    {   CUR = &a; X.loadHostState(); t10::Params P0; P0.persistMs = 0; P0.n = DP_BOOKN; X.cs.bBoard = t10::Board(P0); X.cs.bInit = true; X.saveHostState(); }   // the 30 s entry wait off for the test
    drawOn(X, a, MOCK_CHART);
    std::vector<Op> O = drawOn(X, a, MOCK_CHART);
    DealerProfile::ColsState c = colsOf(X, a);
    if (DUMP) dump(O, a, c);
    const int lqL = c.lay.levels.l + c.keysW;
    check(c.keysW == 170 && c.lay.w[cb::LEVELS] == 340, "column widths unchanged: Key Levels 170 + Liquidity 170 (" + std::to_string(c.keysW) + " / " + std::to_string(c.lay.w[cb::LEVELS]) + ")");
    checkRows(O, c, lqL, "plain", false);
    {   std::vector<std::string> rk; for (const Row& r : rowsOf(O, lqL, c.lay.levels.r)) rk.push_back(r.rk->text); std::sort(rk.begin(), rk.end());
        check(rk == std::vector<std::string>({ "2", "3", "4", "5", "6", "7", "8", "K" }), "rank boxes K + 2..8 (unchanged)"); }
    {   int inCol = 0, out = 0; for (auto& q : O) if (q.kind == "text" && q.l >= lqL && q.text.find('%') != std::string::npos) { if (q.r <= c.lay.levels.r) ++inCol; else ++out; }
        for (auto& q : O) if (q.kind == "text" && q.l >= lqL && q.text.find('%') != std::string::npos) std::printf("  row %s l %d r %d (col r %d)\n", q.text.c_str(), q.l, q.r, c.lay.levels.r);
        check(inCol == 8 && out == 0, "8 rows, every amount inside the Liquidity column (" + std::to_string(inCol) + " / out " + std::to_string(out) + ")"); }
    {   std::vector<sb::Fact> Q; check(sb::readFacts('Q', a.sym, a.spb, MOCK_CHART, dpNowMs(), Q, false) && Q.size() == 8 && Q[0].code == 1 && Q[0].val == 900, "Q facts: the 8 shown nodes, rank 1 = the king (" + std::to_string(Q.size()) + ")"); }
    {   const std::string st = slurp(fl("DealerProfile.status-ES-180.txt"));
        check(st.find("LIQ_BOOK,WHOLE BOOK") != std::string::npos && st.find("shown top 5 + level-linked >= 50% within 1 ATR 3") != std::string::npos && st.find("king 900 lots") != std::string::npos, "status LIQ_BOOK line"); }
    // ---- (2.43.0) the real-liquidity grade: the king (offer 7858) decided 4/4 + HELD; rank 2 (bid 7845) still testing (1:49 left)
    {   auto ctOf = [&](RTDATE d) { time_t tt = (time_t)d; struct tm bt; gmtime_r(&tt, &bt); return fr::ctAbs(bt.tm_year + 1900, bt.tm_mon + 1, bt.tm_mday, bt.tm_hour * 3600 + bt.tm_min * 60 + bt.tm_sec); };
        const size_t nb = a.dt.size(); const long long tTouch = ctOf(a.dt[nb - 15]) - 60, tHeld = ctOf(a.dt[nb - 8]) - 30;   // 45 min ago (a decided grade shows for 60 min)
        std::vector<sb::Fact> SG;
        { sb::Fact f; f.bar = tTouch; f.lo = 7856.5; f.hi = 7859.5; f.side = -1; f.val = 15; f.code = 31; f.state = 1; f.proven = 4; SG.push_back(f); }
        { sb::Fact f; f.bar = tTouch; f.lo = f.hi = 7858.0; f.side = -1; f.val = (double)(tHeld - tTouch); f.code = 32; f.state = 1; f.proven = 1; SG.push_back(f); }
        { sb::Fact f; f.bar = ctOf(a.dt[nb - 1]) + 100; f.lo = 7843.5; f.hi = 7846.5; f.side = 1; f.val = 109; f.code = 30; f.state = 0; SG.push_back(f); }
        sb::postFacts('S', a.sym, a.spb, MOCK_CHART, dpNowMs(), SG); sb::postFacts('B', a.sym, a.spb, MOCK_CHART, dpNowMs(), B);
        std::vector<Op> G = drawOn(X, a, MOCK_CHART); DealerProfile::ColsState cg = colsOf(X, a);
        if (getenv("DP_GRADE_DUMP")) { const char* d0 = DUMP; DUMP = getenv("DP_GRADE_DUMP"); dump(G, a, cg); DUMP = d0; }
        checkRows(G, cg, lqL, "graded (R S notP F + 4/4 + 1:49)", true);
        {   const Op* k4 = nullptr; for (auto& q : G) if (q.kind == "text" && q.text == "4/4" && q.l >= lqL) k4 = &q;
            check(k4 && k4->col == 0x22C55E, "the king's score 4/4 still green"); }
    }
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
