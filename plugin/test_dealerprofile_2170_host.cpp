// test_dealerprofile_2170_host.cpp -- lsDealerProfile 2.17.0 through the REAL DealerProfile.cpp + the real irtsdk.h (the mock style of
// test_dealerprofile_host274.cpp): [1] a status file PER CHART + the old shared one, each chart with its own write gate; [2] the Market
// State SRC GEX from the drawn MenthorQ levels + the grid's hedge lots, LIVE then STALE (Fresh.h), cleared by done().
// Build (from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_2170_host.cpp
//            -Wl,--unresolved-symbols=ignore-all -o t && ./t
// RA1009_test_dealerprofile_klc_twochart.cpp -- regression test for the RA1009 audit fix in DealerProfile 2.7.2.
// Checklist #1: two charts on ONE DealerProfile DLL object must never see each other's native key levels.
// 2.7.1 kept the key-level cache (klc) on the shared extension object, keyed by market only, with a 10-s throttle: a second
// ES chart drawn within 10 s got the FIRST chart's levels. This test runs the REAL DealerProfile.cpp against the REAL irtsdk.h
// with only the SDK calls the key-level path uses mocked (everything else is never executed).
// Build (Linux, from plugin/; the linker flag is required - only the mocked SDK calls are ever executed):
//   g++ -std=c++17 -g -fsanitize=address,undefined -DRA1009_PATCHED -I<sdk include> -I.
//       test_dealerprofile_klc_twochart.cpp -Wl,--unresolved-symbols=ignore-all -o t && ./t
//   (without -DRA1009_PATCHED and with -DDP_SRC='"<old DealerProfile.cpp>"' it shows the 2.7.1 leak: 1 failure)
#ifndef _WIN32
#include "DealerRead_test_linux_sdk_compat.h"   // RT_LONG for the real irtsdk.h on Linux
#endif
#ifndef _WIN32                     // Linux test build: the MS CRT calls DealerProfile.cpp uses (test-only shims)
#include <ctime>
#include <cstring>
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif
static inline int localtime_s(struct tm* r, const time_t* t) { return localtime_r(t, r) ? 0 : 1; }
static inline int strncat_s(char* d, size_t n, const char* s, size_t) { size_t l = strlen(d); if (l + 1 >= n) return 0; strncat(d, s, n - l - 1); return 0; }
static inline int strncpy_s(char* d, size_t n, const char* s, size_t) { if (!n) return 1; strncpy(d, s, n - 1); d[n - 1] = 0; return 0; }
#endif
// the standard headers first, so only DealerProfile's own members are opened up for the test
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
#ifndef DP_SRC
#define DP_SRC "DealerProfile.cpp"
#endif
#include DP_SRC
#undef private
#undef protected
#include <cstdio>
#include <vector>

struct Chart { std::string root, sym; int spb = 180; bool keepsSlot = true; void* user = nullptr;
               std::vector<unsigned long> dt; std::vector<float> o, h, l, c; std::vector<unsigned long> v; };
static Chart* CUR = nullptr;

cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
long cppExtension::getBarCount(void) { return (long)CUR->dt.size(); }
char* cppExtension::getSymbol(void) { return (char*)CUR->sym.c_str(); }
int cppExtension::getSecondsPerBar(void) { return CUR->spb; }
struct tm* cppExtension::getLocaltime(RTDATE d, struct tm* p) { time_t t = (time_t)d; gmtime_r(&t, p); return p; }
void* cppExtension::getUserData(void) { return CUR->user; }
void cppExtension::setUserData(void* p) { if (CUR->keepsSlot) CUR->user = p; }
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


// ---- extra mocks for the badge / settings paths: a 1600 x 800 pane, price -> y linear, bar i -> x = 100 + 4 i
static const float P0 = 6650.0f;                 // price at y = 400
static short yOfP(float p) { return (short)(400 - (p - P0) * 4.0f); }
static std::string PERIOD = "3 Minutes";
static int VS = 0, VE = 0;
struct Drawn { short l, t, r, b; };
static std::vector<Drawn> RECTS;                 // every filled RCT drawn (the badge backgrounds)
static std::vector<std::string> TEXTS;
static int curFont = 10;
cppExtension::FONT::FONT(FONT_ID i, short s, FONT_STYLE st) : id(i), size(s), style(st) {}
char* cppExtension::getRootSymbol(char* b, const char*) { if (b) { strcpy(b, CUR->sym.c_str()); return b; } return (char*)CUR->sym.c_str(); }
char* cppExtension::getPeriodicityLabel(void) { return (char*)PERIOD.c_str(); }
RTX_RESULT cppExtension::getVisibleBars(int* s, int* e) { *s = VS; *e = VE; return RTX_OK; }
double cppExtension::getTickIncrement(const char*) { return 0.25; }   // (2.9.0 test fix) 2.8.7 matches levels on the SDK tick grid - this mock was missing (SEGV)
void cppExtension::setPen(COLOR, short, PEN_STYLE) {}
void cppExtension::setTextColor(COLOR) {}
RTX_RESULT cppExtension::setFont(FONT& f) { curFont = f.size; return RTX_OK; }
int cppExtension::getTextWidth(const char* s, int) { return (int)(strlen(s) * curFont * 6 / 10); }
void cppExtension::RCT::getPaneRect(RTBOOL) { left = 0; top = 0; right = 1600; bottom = 800; }
void cppExtension::RCT::getScaleRect(void) { left = 1600; top = 0; right = 1660; bottom = 800; }
void cppExtension::RCT::set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
void cppExtension::RCT::draw(short, COLOR, COLOR, DRAWTYPE, BRUSH_STYLE) { RECTS.push_back(Drawn{left, top, right, bottom}); }
void cppExtension::RCT::drawText(const char* t, RTBOOL, RTBOOL) { TEXTS.push_back(t); }
RTX_RESULT cppExtension::PNT::set(int bar, float price, BAR_POSITION) { h = (short)(100 + 4 * bar - 4 * (int)(CUR->dt.size() - 300 > 0 ? CUR->dt.size() - 300 : 0)); v = yOfP(price); return RTX_OK; }
void cppExtension::PNT::setDrawPosition() {}
void cppExtension::PNT::drawLineTo() {}

static int passes = 0, fails = 0;
static void check(bool ok, const char* m) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }
#include "tests_dp290/dp_mock_cols.h"   // (CR5200) the 2.11.0 SDK calls (chart options, dialog setters)
static std::string UP;
static std::string fl(const char* name) { return UP + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name; }

static RTDATE NOW_RT = 0;
RTDATE cppExtension::currentDate(void) { return NOW_RT; }
static RTDATE utcAt(int y, int mo, int d, int h, int mi) { return (RTDATE)((long long)svl::daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60); }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bars(Chart& C, float close)
{
    for (unsigned long t = (unsigned long)utcAt(2026, 10, 19, 8, 33); t <= (unsigned long)utcAt(2026, 10, 19, 10, 0); t += 180) {
        C.dt.push_back(t); C.o.push_back(close); C.h.push_back(close + 1); C.l.push_back(close - 1); C.c.push_back(close); C.v.push_back(10);
    }
}
int main()
{
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp2170XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    {   // ---- 1. per-chart status files
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; bars(a, 7703.0f); CUR = &a; MOCK_CHART = "ES 3m";
        X.loadHostState(); X.mkt = "ES"; X.root = "EPZ26"; X.cs.spb = 180; X.writeStatus("drawn"); X.saveHostState();
        Chart b; b.root = "CL"; b.sym = "CLX6"; bars(b, 94.0f); CUR = &b; MOCK_CHART = "CL 3m";
        X.loadHostState(); X.mkt = "CL"; X.root = "CLX6"; X.cs.spb = 180; X.writeStatus("no data"); X.saveHostState();
        const std::string es = slurp(fl("DealerProfile.status-ES-180.txt")), cl = slurp(fl("DealerProfile.status-CL-180.txt")), old = slurp(fl("DealerProfile.status.txt"));
        check(es.find("VERSION,2.17.0") != std::string::npos && es.find("MARKET,ES") != std::string::npos && es.find("STATE,drawn") != std::string::npos, "ES chart: its own status file");
        check(cl.find("MARKET,CL") != std::string::npos && cl.find("STATE,no data") != std::string::npos, "CL chart: its own status file (the ES one is not overwritten)");
        check(old.find("MARKET,CL") != std::string::npos, "the old shared DealerProfile.status.txt is still written (one more version)");
        // ES writes again with the same content: its gate holds (no rewrite); the CL chart's write did not reset the ES gate
        std::remove(fl("DealerProfile.status-ES-180.txt").c_str()); CUR = &a; MOCK_CHART = "ES 3m";
        X.loadHostState(); X.writeStatus("drawn"); X.saveHostState();
        check(slurp(fl("DealerProfile.status-ES-180.txt")).empty(), "same content within a minute: not rewritten (this chart's own gate)");
        X.loadHostState(); X.writeStatus("no data"); X.saveHostState();
        check(slurp(fl("DealerProfile.status-ES-180.txt")).find("STATE,no data") != std::string::npos, "changed content: rewritten");
        X.loadHostState(); X.done(); CUR = &b; X.loadHostState(); X.done();
    }
    {   // ---- 2. the Market State, SRC GEX
        { std::ofstream f(fl("MenthorQLevels.csv").c_str());
          f << "SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH\nEPZ26,7795.000000,CR 0D / GW 0D,48340,2\nEPZ26,7700.000000,PS 0D,48340,2\nEPZ26,7780.000000,HVL 0D,48340,2\nEPZ26,7750.000000,G1,2712319,1\n"; }
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; bars(a, 7703.0f); CUR = &a; MOCK_CHART = "ES 3m";
        X.loadHostState(); X.mkt = "ES"; X.root = "EPZ26"; X.D.chartSym = "EPZ26"; X.loadMQLevels();
        dpaudit::NodeX n1; n1.k = 7700; n1.absG = 3e8; n1.hasD = true; n1.d = -560; X.nx.push_back(n1);
        dpaudit::NodeX n2; n2.k = 7750; n2.absG = 1e8; n2.hasD = true; n2.d = 1200; X.nx.push_back(n2);
        X.bookAt.so = 9 * 3600 + 55 * 60; X.bookAt.y = 2026; X.bookAt.mo = 10; X.bookAt.d = 19;
        X.cs.sym = "EPZ26"; X.cs.spb = 180; X.cs.chart = MOCK_CHART;
        NOW_RT = utcAt(2026, 10, 19, 10, 1);
        X.postMst();
        mst::Facts F = mst::read("GEX", "EPZ26", 180, "ES 3m", -1);
        check(F.ok, "posted");
        check(F.bar == (long long)a.dt[a.dt.size() - 2], "for the last CLOSED bar (9:57)");
        check(mst::getS(F, "regime") == "neg" && mst::getS(F, "hvl") == "7780.00" && mst::getS(F, "cr") == "7795.00" && mst::getS(F, "ps") == "7700.00", "regime neg vs the drawn HVL 7780; CR / PS");
        check(mst::getS(F, "near_level") == "PS0@7700.00" && mst::getS(F, "near_dist_ticks") == "12" && mst::getS(F, "near_lots") == "-560", "nearest PS0, 12 ticks, dealers sell 560 there");
        check(mst::getS(F, "stale") == "0" && mst::getS(F, "data_age_min") == "6", "live, 6 min old");
        NOW_RT = utcAt(2026, 10, 19, 10, 30);                  // 35 min old in MenthorQ hours: STALE (slow state: re-posted at once)
        X.postMst();
        F = mst::read("GEX", "EPZ26", 180, "ES 3m", -1);
        check(mst::getS(F, "stale") == "1" && mst::getS(F, "regime") == "?" && mst::getS(F, "near_lots") == "?", "35 min old: stale = 1, no regime / lots posted as live");
        X.saveHostState(); X.loadHostState(); X.done();
        check(!mst::read("GEX", "EPZ26", 180, "ES 3m", -1).ok, "done(): cleared");
    }
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
