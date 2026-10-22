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
int cppExtension::getListIndex(int) { return -1; }
RTX_RESULT cppExtension::invalidateChart(RTBOOL) { return RTX_OK; }
cppExtension::CBRUSH::CBRUSH(COLOR c, BRUSH_STYLE s) : color(c), style(s) {}
void cppExtension::CBRUSH::set() { brush = color; }
void cppExtension::RCT::getPaneRect(RTBOOL) { left = 0; top = 0; right = 1600; bottom = PH; }
void cppExtension::RCT::getScaleRect(void) { left = 1600; top = 0; right = 1660; bottom = PH; }
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
static std::string UP;
static std::string fl(const char* name) { return UP + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void put(const char* name, const std::string& body) { std::ofstream f(fl(name).c_str(), std::ios::binary); f << body; }

static Chart loadBars(const std::string& D, const char* cut)
{
    Chart a; a.root = "ES"; a.sym = "ESZ6";
    std::istringstream in(slurp(D + "/LRA-IRT-Bars-ES.csv")); std::string ln; std::getline(in, ln);
    while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() < 6) continue;
        if (c[0].substr(11, 5) > std::string(cut)) break;
        struct tm t; memset(&t, 0, sizeof(t)); strptime(c[0].c_str(), "%Y-%m-%d %H:%M:%S", &t); time_t u = timegm(&t);
        a.dt.push_back((unsigned long)u); a.o.push_back(std::stof(c[1])); a.h.push_back(std::stof(c[2])); a.l.push_back(std::stof(c[3])); a.c.push_back(std::stof(c[4])); a.v.push_back(std::stoul(c[5])); }
    return a;
}
// one draw of the real plugin; returns the recorded calls
static std::vector<Op> drawOnce(const std::string& D, const char* cut, float mouseK, const std::string& dealerBody, DealerProfile** keep = nullptr)
{
    for (const char* n : { "MenthorQLevels.csv", "LRA-Levels-ES.csv", "LRA-Touch-ES.csv" }) put(n, slurp(D + "/" + n));
    put("LRA-Dealer-ES.csv", dealerBody);
    put("DeltaProfile.status.txt", "WIDTH,50\nPLACE,Right\n");
    static Chart a; a = loadBars(D, cut);
    NOW = a.dt.back() + 180; CUR = &a;
    DealerProfile* X = new DealerProfile();
    X->loadHostState();
    DealerProfile::KLC K; K.n = (long)a.dt.size(); K.lastT = (long long)a.dt.back(); K.at = time(nullptr);
    { std::istringstream in(slurp(D + "/LRA-KeyLevels-ES.csv")); std::string ln;
      while (std::getline(in, ln)) { std::vector<std::string> c = dl::split(ln, '|'); if (c.size() >= 4 && c[0] == "L") K.v.push_back(klv::Merged{ c[1], std::stod(c[2]), c[3] == "1" }); } }
    X->klc["ES"] = K;
    X->saveHostState();
    if (mouseK > 0) {   // the mouse over the Profile at that strike (the bar index right of the last bar = the profile column)
        RTX_EVENT e; memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.price = mouseK; e.v.mouse.v = yOfP(mouseK); e.v.mouse.h = 1590;
        e.v.mouse.bar = (long)((1590 - X0) / PPB);
        X->mouse(&e);
    }
    OPS.clear();
    X->draw();
    std::vector<Op> r = OPS;
    if (keep) *keep = X; else { X->done(); delete X; }
    return r;
}
static bool hasText(const std::vector<Op>& O, const std::string& s, unsigned long col = 0xFFFFFFFF)
{ for (size_t i = 0; i < O.size(); i++) if (O[i].kind == "text" && O[i].text == s && (col == 0xFFFFFFFF || O[i].col == col)) return true; return false; }
static bool hasTextPrefix(const std::vector<Op>& O, const std::string& s)
{ for (size_t i = 0; i < O.size(); i++) if (O[i].kind == "text" && O[i].text.rfind(s, 0) == 0) return true; return false; }

static void logicTests()
{
    using namespace dpaudit;
    NodeX x;
    check(parseNodeX("NODEX|7900.00|27211419|21|-8591|-3393342812", x) && x.k == 7900.0f && x.absG == 27211419 && x.hasChg && x.chg == 21 && x.hasD && x.d == -8591, "NODEX parsed");
    check(parseNodeX("NODEX|7900.00|27211419||||", x) && !x.hasChg && !x.hasD, "NODEX with no % / hedge");
    check(!parseNodeX("NODEX|7900|nan|1|2|3", x) && !parseNodeX("NODEX|7900|-5|1|2|3", x) && !parseNodeX("NODEX|7900|5|1x|2|3", x) && !parseNodeX("NODEX|7900|5", x), "malformed NODEX refused");
    BookAsOf b;
    check(parseBookAsOf("BOOKASOF|17700|2026-10-13", b) && b.so == 17700 && b.y == 2026 && b.d == 13, "BOOKASOF parsed");
    check(parseBookAsOf("BOOKASOF||", b) && !b.ok(), "BOOKASOF with no grid");
    check(!parseBookAsOf("BOOKASOF|90000|2026-10-13", b) && !parseBookAsOf("BOOKASOF|12|2026-13-01", b), "bad BOOKASOF refused");
    check(clock12(17700) == "4:55 AM" && clock12(0) == "12:00 AM" && clock12(12 * 3600) == "12:00 PM" && clock12(14 * 3600 + 57 * 60) == "2:57 PM", "12-hour Central clock");
    setenv("TZ", "UTC", 1); tzset();
    struct tm n; memset(&n, 0, sizeof(n)); n.tm_year = 126; n.tm_mon = 9; n.tm_mday = 13; n.tm_hour = 15; n.tm_min = 0;
    parseBookAsOf("BOOKASOF|17700|2026-10-13", b);
    const double age = ageMinutes(b, n, 0);
    check(std::fabs(age - 605) < 0.5, "age 4:55 AM -> 3:00 PM = 605 min");
    check(bookStale(age, mqUpdateHours(2, 900)) && !bookStale(15, true) && !bookStale(age, mqUpdateHours(2, 17 * 60)), "STALE: > 20 min while MenthorQ updates only");
    check(!mqUpdateHours(6, 600) && !mqUpdateHours(0, 20 * 60) && mqUpdateHours(0, 21 * 60 + 45) && !mqUpdateHours(5, 16 * 60 + 30) && mqUpdateHours(1, 3 * 60), "MenthorQ update hours (Sun 21:40 - Fri 16:00, paused 16:00-21:40)");
    check(bookLine(b, age, true, "ES", "ES") == "STALE 10h - 4:55 AM", "STALE line");
    check(bookLine(b, 5, false, "SPX", "ES") == "SPX book 4:55 AM" && bookLine(b, 5, false, "ES", "ES") == "book 4:55 AM", "fresh line, index book named");
    check(wfLetter(1) == 'W' && wfLetter(-1) == 'F' && wfLetter(0) == 'W', "W = net long gamma, F = net short");
    check(mutedNode(33.8e6, -2.1e6) && !mutedNode(27.2e6, 26e6), "big absolute, near-zero net = muted");
    check(deltaNum(-599) == "-599" && deltaNum(599) == "599" && deltaNum(-5300) == "-5.3K" && deltaNum(12755) == "12.8K" && deltaNum(-128000) == "-128K", "hedge numbers");
    check(pctChg(7) == "+7%" && pctChg(-23) == "-23%" && pctChg(0) == "0%", "% change text");
    check(usdShort(27.2e6) == "$27.2M" && usdShort(2.1e9) == "$2.1B" && usdShort(-2.1e6) == "-$2.1M", "$ text");
    NodeX a; a.absG = 40; a.hasD = true; a.d = -10; NodeX c = a; c.absG = 13;
    check(showDelta(a, 100) && !showDelta(c, 100), "hedge number only on strikes >= 35% of the biggest wall");
    std::vector<NodeX> v(5); for (int i = 0; i < 5; i++) { v[i].k = 100.0f + i; v[i].absG = (i == 2 ? 50 : i * 10); }
    std::vector<size_t> idx = {0, 1, 2, 3, 4}; std::vector<size_t> t3 = top3(v, idx);
    check(t3.size() == 3 && t3[0] == 2 && t3[1] == 4 && t3[2] == 3, "top 3 by absolute GEX");
    std::vector<Piece> P(3); P[0].w = 10; P[1].w = 40; P[2].w = 40;
    placePieces(P, 1500, 1420, 5);
    check(P[0].shown && P[1].shown && !P[2].shown && P[1].x >= 1420, "a piece that would cross the column's left limit is dropped");
    check(mqShowName("CR 0D / GW 0D") == "CR0" && mqShowName("CR 0D / GW 0D STALE 8h") == "CR0 STALE 8h" && mqShowName("PS 0D") == "PS 0D", "CR0 + GW0 merged");
    check(mqHiddenGex("G2") && mqHiddenGex("G10") && mqHiddenGex("G3 STALE 2h") && !mqHiddenGex("G1") && !mqHiddenGex("HVL 0D") && !mqHiddenGex("G1/G2"), "G2-G10 hidden, G1 kept");
    check(onKeyLevel(7834.5, {7834.5}, 0.25) && !onKeyLevel(7835.0, {7834.5}, 0.25), "on a key level = within half a tick");
    NodeX h; parseNodeX("NODEX|7900.00|27211419|21|-8591|-3393342812", h);
    Hover H = nodeHover("ES", h, 26e6, 40e6, 7857);
    check(H.title == "What do dealers do if ES gets to 7900?" && H.lines.size() >= 5, "node hover: the question first");
    bool usd = false, how = false, ex = false, sell = false;
    for (size_t i = 0; i < H.lines.size(); i++) { if (H.lines[i].find("$27.2M") != std::string::npos) usd = true; if (H.lines[i].rfind("How to use", 0) == 0) how = true;
        if (H.lines[i].rfind("Example", 0) == 0) ex = true; if (H.lines[i].find("SELL 8.6K ES (about $3.4B)") != std::string::npos) sell = true; }
    check(usd && how && ex && sell, "node hover: the GEX value, how to use, an example, the hedge in contracts and $");
    Hover V = hvlHover("ES", 7850); check(V.lines.size() == 4 && V.title.find("regime") != std::string::npos, "HVL hover");
    std::vector<std::string> w = wrapPx("one two three four five", 30, [](const std::string& s) { return (int)s.size() * 3; });
    check(w.size() == 3 && w[0] == "one two", "word wrap");
}

int main(int argc, char** argv)
{
    logicTests();
    const char* opsOut = argc > 1 ? argv[1] : nullptr;
    const std::string D = argc > 2 ? argv[2] : "tests_dp290";
    const char* cut = argc > 3 ? argv[3] : "14:57";
    const float mouseK = argc > 4 ? std::stof(argv[4]) : 0.0f;
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp290hostXXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    const std::string dealer = slurp(D + "/LRA-Dealer-ES.csv");
    check(!dealer.empty() && dealer.find("NODEX|") != std::string::npos, "fixture: the bridge's file 2.4 (NODEX rows)");

    std::vector<Op> O = drawOnce(D, cut, 0, dealer);
    // (2.11.0, CR5200) the Dealer column = the right 80 px before the axis; the header fits it ("DP v2.11.0", "STALE 10h")
    const int anchor = 1598, reach = 80, bound = 1600 - reach;   // the mock scale starts at the pane edge (1600): the column is 1520-1600
    check(hasText(O, std::string("DP v") + DP_VERSION), "header: the version (short form inside the 80 px column)");   // (2026-10-22) the current DP_VERSION, not the 2.12.0 literal
    check(hasText(O, "STALE 10h"), "header: STALE 10h (the time is in the header hover)");
    {   // (2.11.1, MQF100 2026-10-14) WHY it is stale + what is being done, from the bridge's LRA-MQFeed.txt
        using namespace dpaudit;
        const long long nowU = (long long)time(nullptr);
        auto feedTxt = [&](const char* st, const char* code, const char* sh, const char* retry, long long at) {
            return std::string("VERSION|1\nFEED|") + st + "|125|" + code + "|" + sh + "|" + retry + "|9:35 AM|" + std::to_string(at) + "\n"; };
        auto lines = [](const std::string& t) { std::vector<std::string> v; std::istringstream in(t); std::string l; while (std::getline(in, l)) v.push_back(l); return v; };
        FeedStat f = parseFeed(lines(feedTxt("STALE", "no-network", "no internet", "retrying every minute", nowU)));
        check(f.ok && f.stale && f.age == 125 && f.code == "no-network" && f.grid == "9:35 AM", "feed file parsed");
        auto wide = [](const std::string&) { return true; }; auto narrow = [](const std::string& t) { return t.size() <= 12; };
        check(feedLines(f, nowU, wide) == std::vector<std::string>{ "no internet - retrying" }, "wide column: one line 'no internet - retrying'");
        check(feedLines(f, nowU, narrow) == std::vector<std::string>{ "no internet", "retrying" }, "80 px column: 'no internet' / 'retrying'");
        check(feedLines(f, nowU + 301, wide).empty(), "a feed file older than 5 min (bridge stopped): nothing shown");
        FeedStat so = parseFeed(lines(feedTxt("STALE", "signed-out", "signed out", "you must sign in", nowU)));
        check(feedLines(so, nowU, wide) == std::vector<std::string>{ "signed out - sign in" }, "signed out: says what he must do");
        check(feedLines(parseFeed(lines(feedTxt("OK", "ok", "live", "", nowU))), nowU, wide).empty(), "fresh grids: no extra line");
        check(!parseFeed(lines("VERSION|2\nFEED|STALE|1|a|b|c|d|" + std::to_string(nowU) + "\n")).ok, "wrong VERSION: ignored");
        check(!parseFeed(lines("VERSION|1\nFEED|WHAT|1|a|b|c|d|" + std::to_string(nowU) + "\n")).ok, "garbled state: ignored");
        check(!parseFeed(lines("VERSION|1\nFEED|STALE|x|a|b|c|d|" + std::to_string(nowU) + "\n")).ok, "garbled age: ignored");
        check(!parseFeed(lines("VERSION|1\nFEED|STALE|1|a|b|c|d|12x\n")).ok && !parseFeed(lines("FEED|STALE|1|a|b|c|d|5\n")).ok, "garbled time / no VERSION: ignored");
        Hover h; h.title = "t"; h.lines.push_back("first");
        feedHover(h, f, nowU);
        check(h.lines.size() == 4 && h.lines[1].find("Why: no internet") == 0 && h.lines[2] == "What is being done: retrying every minute." && h.lines[3].find("You: nothing") == 0, "header hover: why / what is being done / what you do");
        // on the chart: the stale ES grid + a fresh feed file -> the reason under "STALE 10h", inside the Dealer column
        put("LRA-MQFeed.txt", feedTxt("STALE", "no-network", "no internet", "retrying every minute", nowU));
        std::vector<Op> F = drawOnce(D, cut, 0, dealer);
        check(hasText(F, "STALE 10h") && hasText(F, "no internet") && hasText(F, "retrying"), "chart: 'STALE 10h' / 'no internet' / 'retrying'");
        int out = 0; for (auto& q : F) if (q.kind == "text" && (q.text == "no internet" || q.text == "retrying") && (q.l < bound || q.r > 1600)) out++;
        check(out == 0, "the reason lines stay inside the Dealer column");
        put("LRA-MQFeed.txt", feedTxt("STALE", "no-network", "no internet", "retrying every minute", nowU - 3600));
        F = drawOnce(D, cut, 0, dealer);
        check(hasText(F, "STALE 10h") && !hasText(F, "no internet"), "an old feed file (bridge not running): only STALE, no guessed reason");
        put("LRA-MQFeed.txt", "garbage|||\n");
        F = drawOnce(D, cut, 0, dealer);
        check(hasText(F, "STALE 10h") && !hasText(F, "no internet"), "a garbled feed file: only STALE");
        put("LRA-MQFeed.txt", "");
    }
    int small = 0, crossing = 0, gexOnFace = 0, wN = 0, fN = 0, redNum = 0, greenNum = 0, pct = 0, dashed = 0;
    for (size_t i = 0; i < O.size(); i++) {
        const Op& q = O[i];
        if (q.kind == "line" && q.style != 0) dashed++;
        if (q.kind == "text") {
            if (q.font < 10 && !(q.font == 9 && q.r < bound)) small++;   // (2.11.0) the Levels column badges are 9 pt (approved mockup)
            if (q.l < bound && q.r > bound) crossing++;
            if (q.l >= bound) {
                const std::string& t = q.text;
                if (t.size() > 1 && (t.back() == 'M' || t.back() == 'B') && std::isdigit((unsigned char)t[0])) gexOnFace++;
                if (t.rfind("B ", 0) == 0) wN++; if (t.rfind("S ", 0) == 0) fN++;   // (2.11.0) the hedge text inside the nodes
                if (q.col == (C_SELL & 0xFFFFFF) && t[0] == '-') redNum++;
                if (q.col == (C_BUY & 0xFFFFFF) && std::isdigit((unsigned char)t[0])) greenNum++;
                if (t.back() == '%' && (t[0] == '+' || t[0] == '-' || t[0] == '0')) pct++;
            }
        }
    }
    check(small == 0, "every text 10 pt or larger (" + std::to_string(small) + " smaller)");
    check(crossing == 0, "no text crosses the Profile's REACH edge (the Delta Profile column)");
    check(gexOnFace == 0, "no GEX value on the face");
    { int faded = 0; for (size_t i = 0; i < O.size(); i++) if (O[i].kind == "box" && O[i].r == anchor && O[i].t > 40 && (O[i].col == dcol::C_POS || O[i].col == dcol::C_NEG)) faded++;
      check(faded == 0, "(2.11.0) a STALE grid: every node faded"); }
    (void)redNum; (void)greenNum;
    check(pct == 0, "(2.11.0) no % pills on the face (in the node hover)");
    check(dashed == 0, "no dashed lines");
    check(!hasTextPrefix(O, "+GAMMA above") && hasText(O, "HVL"), "(2.11.0) no HVL regime tags on the chart; the HVL badge is in the Levels column");
    check(hasText(O, "CR0") && !hasText(O, "CR 0D / GW 0D"), "CR 0D / GW 0D drawn as one CR0");
    check(!hasText(O, "G2"), "G2 (7,800 - not on a key level) hidden");
    check(hasText(O, "G1"), "G1 kept");
    // node bars: the biggest absolute GEX strike (7,800 here, 40.5M) gets the full width
    { int full = 0; for (size_t i = 0; i < O.size(); i++) if (O[i].kind == "box" && O[i].r == anchor && O[i].r - O[i].l == 50) full++;
      (void)full; }
    { int full = 0; for (size_t i = 0; i < O.size(); i++) if (O[i].kind == "box" && O[i].r == anchor && O[i].r - O[i].l == 80 - 6) full++;
      check(full >= 1, "(2.11.0) the biggest wall = the full Dealer column (80 px - 6)"); }
    // the hover: the mouse over the 7,900 node
    std::vector<Op> H = drawOnce(D, cut, 7900.0f, dealer);
    // (2.17.3, Rassul 2026-10-18 23:37 CT "get rid of the hover over on the gex levels and other places") every hover OFF (DP_HOVERS = false)
    check(!hasText(H, "What do dealers do if ES gets to 7900?"), "(2.17.3) no node hover drawn with the mouse over a node");
    int hovBoxes = 0;
    for (size_t i = 0; i < H.size(); i++) if (H[i].kind == "box" && H[i].col == 0x111827) ++hovBoxes;
    check(hovBoxes == 0, "(2.17.3) no hover box");
    // a fresh grid: no STALE, nodes not faded
    {
        std::string fresh = dealer; size_t p = fresh.find("BOOKASOF|17700"); fresh.replace(p, 14, "BOOKASOF|53700");   // 14:55
        std::vector<Op> F = drawOnce(D, cut, 0, fresh);
        check(!hasTextPrefix(F, "STALE") && (hasText(F, "book 2:55 PM") || hasText(F, "2:55 PM")), "fresh grid: 'book 2:55 PM' (or '2:55 PM' in a narrow column), no STALE");
        { int teal = 0, orange = 0, past = 0; for (size_t i = 0; i < F.size(); i++) if (F[i].kind == "box" && F[i].r == anchor && F[i].t > 40) {
              if (F[i].col == dcol::C_POS) teal++; if (F[i].col == dcol::C_NEG) orange++; if (F[i].l < bound) past++; }
          check(teal > 0 && orange > 0 && past == 0, "(2.11.0) fresh grid: nodes teal (+) / orange (-), anchored on the column's right edge, never past its left edge"); }
        int red = 0, green = 0;
        for (size_t i = 0; i < F.size(); i++) if (F[i].kind == "text" && F[i].l >= bound) {
            // (DP 2.16.0, Rassul 2026-10-16 22:16 "just give the number followed by lots") "480 lots": the node colour says buy / sell
            const std::string& t = F[i].text;
            if (t.size() > 5 && t.compare(t.size() - 5, 5, " lots") == 0) red++; }
        check(red + green > 0, "fresh grid: 'N lots' hedge text inside the nodes (" + std::to_string(red + green) + ")");
    }
    // an old 2.3 file (no NODEX): the 2.8.7 drawing, no header
    {
        std::string old; std::istringstream in(dealer); std::string ln;
        while (std::getline(in, ln)) if (ln.rfind("NODEX|", 0) != 0) old += ln + "\n";
        std::vector<Op> F = drawOnce(D, cut, 0, old);
        check(!hasText(F, "DP v2.11.0") && !hasTextPrefix(F, "B ") && !hasTextPrefix(F, "S "), "file 2.3: drawn as 2.8.7");
    }
    // a malformed NODEX row: the snapshot is refused (nothing torn drawn)
    {
        std::string bad = dealer + "NODEX|7900|nan|||\n";
        std::vector<Op> F = drawOnce(D, cut, 0, bad);
        check(!hasTextPrefix(F, "B ") && !hasTextPrefix(F, "S "), "a malformed NODEX row rejects the file snapshot");
    }
    if (opsOut) {
        std::vector<Op> R = mouseK > 0 ? drawOnce(D, cut, mouseK, dealer) : drawOnce(D, cut, 0, dealer);
        FILE* f = std::fopen(opsOut, "w");
        std::fprintf(f, "pane|0|0|1600|%d|1660\n", PH);
        Chart a = loadBars(D, cut);
        for (size_t i = 0; i < a.dt.size(); i++) std::fprintf(f, "bar|%d|%d|%d|%d|%d|%lu\n", X0 + PPB * (int)i, yOfP(a.o[i]), yOfP(a.h[i]), yOfP(a.l[i]), yOfP(a.c[i]), a.dt[i]);
        for (int p = 7800; p <= 7930; p += 10) std::fprintf(f, "tick|%d|%d\n", yOfP((float)p), p);
        for (size_t i = 0; i < R.size(); i++) {
            const Op& q = R[i];
            if (q.kind == "text") std::fprintf(f, "text|%d|%d|%d|%d|%06lx|%d|%d|%s\n", q.l, q.t, q.r, q.b, q.col, q.font, q.bold ? 1 : 0, q.text.c_str());
            else if (q.kind == "line") std::fprintf(f, "line|%d|%d|%d|%d|%06lx|%d\n", q.l, q.t, q.r, q.b, q.col, q.font);
            else std::fprintf(f, "%s|%d|%d|%d|%d|%06lx|%06lx|%d\n", q.kind.c_str(), q.l, q.t, q.r, q.b, q.col, q.col2, q.font);
        }
        std::fclose(f);
        (void)pOfY;
    }
    std::string rm = std::string("rm -rf ") + dir; if (std::system(rm.c_str()) != 0) {}
    std::printf("RESULT %d passed, %d failed\n", PASSES, FAILS);
    return FAILS ? 1 : 0;
}
