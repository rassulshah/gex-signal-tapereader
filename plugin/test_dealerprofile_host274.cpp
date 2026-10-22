// test_dealerprofile_host274.cpp -- DealerProfile 2.7.4 fixes run through the REAL DealerProfile.cpp + real irtsdk.h, with the SDK
// calls the level / badge / settings paths use mocked (same mock style as test_dealerprofile_klc_twochart.cpp).
// Build: g++ -std=c++17 -g -fsanitize=address,undefined -I<sdk include> -I. test_dealerprofile_host274.cpp
//            -Wl,--unresolved-symbols=ignore-all -o t && ./t
// (the original header of the two-chart test follows, for its notes)
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
#include <thread>
#include <chrono>
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

#include "tests_dp290/dp_mock_vap.h"   // (DP 2.18.0) the volume-at-price calls of the key levels
static int passes = 0, fails = 0;
static void check(bool ok, const char* m) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }
#include "tests_dp290/dp_mock_cols.h"   // (CR5200) the 2.11.0 SDK calls (chart options, dialog setters)
static std::string UP;
static std::string fl(const char* name) { return UP + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name; }

// two Globex sessions of 3-min ES bars: Wed 17:00 .. Thu 16:00 (RTH high = base + 10) then Thu 17:00 .. Fri 10:00
static void build(Chart& C, float base)
{
    const unsigned long wed1700 = (unsigned long)svl::daysFromCivil(2026, 10, 7) * 86400UL + 17UL * 3600UL;
    for (unsigned long t = wed1700 + 180; t <= wed1700 + 23UL * 3600UL; t += 180) {
        int sec = (int)(t % 86400); float hi = base + 1, lo = base - 1;
        if (sec == 10 * 3600) hi = base + 10;
        C.dt.push_back(t); C.o.push_back(base); C.h.push_back(hi); C.l.push_back(lo); C.c.push_back(base); C.v.push_back(10);
    }
    const unsigned long thu1700 = wed1700 + 86400UL;
    for (unsigned long t = thu1700 + 180; t <= thu1700 + 17UL * 3600UL; t += 180) {
        C.dt.push_back(t); C.o.push_back(base); C.h.push_back(base + 1); C.l.push_back(base - 1); C.c.push_back(base); C.v.push_back(10);
    }
}
static bool overlap(const Drawn& a, const Drawn& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    char tmpl[] = "/tmp/dp274XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 2;
    UP = std::string(dir) + "/u"; setenv("USERPROFILE", UP.c_str(), 1);
    {   // ---- 1. key-level badges carry their price: "<price> <NAME>  <odds>"
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6650.0f); CUR = &a;
        VE = (int)a.dt.size() - 1; VS = VE - 199;          // bars at x 500..1296: room on the left
        X.loadHostState(); X.mkt = "ES"; X.root = "EPZ26"; X.pend.clear();
        X.drawKeyLevels(X.cfg);
        bool pdh = false;
        for (auto& P : X.pend) { std::string all; for (auto& s : P.seg) all += s.s;
            if (all.rfind("6660.00 PFH \xB7 PDH", 0) == 0) pdh = true;      // PFH = PDH here: one merged badge
            check(P.seg.size() >= 2 && P.seg[1].bold && !P.seg[0].bold, "badge: price plain, name bold"); }
        check(pdh, "ES key-level badge reads \"6660.00 PFH . PDH...\" (2.10.0: middle dot on the badge)");
        // ---- 2. drawn: badges centred, never over each other, the Liquidity column (< 110) or past maxR (2.8.2: price bars allowed)
        if (getenv("DBG")) for (auto& P : X.pend) { std::string all; for (auto& s : P.seg) all += s.s; std::printf("pend y=%d %s\n", P.y, all.c_str()); }
        // (2.11.0, CR5200) the compact badges sit in the LEVELS column right of the last bar (ColumnBoard layout), never overlapping
        { cppExtension::RCT pane; pane.getPaneRect(false); X.columns(pane, X.rightEdge(pane), true); }
        RECTS.clear(); X.flushLabels(X.cfg);
        if (getenv("DBG")) for (auto& r : RECTS) std::printf("rect %d %d %d %d\n", r.l, r.t, r.r, r.b);
        check(!RECTS.empty(), "badges drawn");
        const bp::Box col = X.cs.lay.levels;
        std::printf("  badges %zu in the Levels column %d-%d\n", RECTS.size(), col.l, col.r);
        for (size_t i = 0; i < RECTS.size(); i++) {
            check(RECTS[i].l >= col.l && RECTS[i].r <= col.r, "badge inside the Levels column");
            for (size_t j = i + 1; j < RECTS.size(); j++) check(!overlap(RECTS[i], RECTS[j]), "badges never overlap each other");
        }
        X.saveHostState(); X.done();
    }
    {   // ---- 3. MenthorQ: a continuous ES chart picks the FRONT contract (not EPH27 by alphabet), green / red by level type
        std::ofstream f(fl("MenthorQLevels.csv").c_str());
        f << "SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH\nEPH27,6700.000000,CR 0D,48340,2\nEPZ26,6640.000000,CR 0D / GW 0D,48340,2\nEPZ26,6630.000000,PS 0D,48340,2\n";
        f.close();
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "ES"; build(a, 6650.0f); CUR = &a;
        X.loadHostState(); X.mkt = "ES"; X.root = "ES"; X.pend.clear();
        X.drawMQLevels(X.cfg);
        bool front = false, back = false; COLOR cr = 0, ps = 0;
        for (auto& P : X.pend) { std::string all; for (auto& s : P.seg) all += s.s;
            if (all.rfind("6640.00 CR0", 0) == 0) { front = true; cr = P.seg[1].c; }   // (2.9.0) "CR 0D / GW 0D" shows as one "CR0"
            if (all.rfind("6630.00 PS 0D", 0) == 0) ps = P.seg[1].c;
            if (all.find("6700.00") != std::string::npos) back = true; }
        check(front && !back, "continuous ES: the Z26 levels, not the H27 (back month) ones");
        check(cr == C_SELL && ps == C_BUY, "CR red (resistance), PS green (support)");
        X.done();
        // a chart on the next month (roll): the file's Z26 levels are drawn at strike + the contract offset
        Chart b; b.root = "ESM7"; b.sym = "ESM7"; build(b, 6710.0f); CUR = &b;
        X.loadHostState(); X.mkt = "ES"; X.root = "ESM7"; X.off = 60.0f; X.D.chartSym = "EPZ26"; X.pend.clear();
        X.drawMQLevels(X.cfg);
        bool shifted = false; for (auto& P : X.pend) if (P.seg.size() > 1 && P.seg[1].s == "PS 0D" && P.y == yOfP(6690.0f)) shifted = true;
        check(shifted, "roll (Rithmic ESM7, file on EPZ26): PS 6630 drawn at 6690 (strike + offset)");
        X.done();
        Chart c; c.root = "ESH7"; c.sym = "ESH7"; build(c, 6710.0f); CUR = &c;      // Rithmic ESH7 = CQG EPH27: its own levels, unshifted
        X.loadHostState(); X.mkt = "ES"; X.root = "ESH7"; X.off = 60.0f; X.D.chartSym = "EPZ26"; X.pend.clear();
        X.drawMQLevels(X.cfg);
        bool own = false; for (auto& P : X.pend) if (P.seg.size() > 1 && P.seg[0].s == "6700.00 " && P.y == yOfP(6700.0f)) own = true;
        check(own && X.pend.size() == 1, "Rithmic ESH7: the EPH27 levels at their strikes");
        X.done();
    }
    {   // ---- 5. (2.11.0, Rassul 11:39 CT) liquidity end to end: the Liquidity Profile's LRA-LiqLevels file -> the badge's block + hover
        { std::ofstream f(fl("LRA-LiqLevels-ES.csv").c_str(), std::ios::binary);
          f << "LIQL,1," << (long long)time(nullptr) << ",0.25,2.5,2.1,4\r\nL,26640,R,977,2.6\r\nL,26560,S,812,1.4\r\n"; }   // 6660.00 offers 2.6x; 6640 bids 1.4x
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6650.0f); CUR = &a;
        VE = (int)a.dt.size() - 1; VS = VE - 199;
        X.loadHostState(); X.mkt = "ES"; X.root = "EPZ26"; X.pend.clear();
        X.drawKeyLevels(X.cfg);
        bool tag = false; for (auto& P : X.pend) { std::string all; for (auto& s : P.seg) all += s.s;
            if (getenv("DBG")) std::printf("liq pend %s lots %lld hl %d\n", all.c_str(), P.lots, (int)P.hl);
            if (all.rfind("6660.00 PFH \xB7 PDH", 0) == 0 && P.haveLiq && P.lots == 977 && P.hl && !P.bid && P.liq.find("977 contracts resting offers (2.6x") == 0) tag = true; }
        check(tag, "(2.11.0) key level on 977 resting offers at 2.6x: HL block, red side, hover text");
        X.done();
    }
    {   // ---- 4. settings: a chart without a saved row does not re-read the settings file on every repaint
        std::ofstream f(fl("DealerProfile.settings.txt").c_str()); f << "EPZ26~3 Minutes*~|0|50|9|0|0|0|0|0|cob_bull,cob_bear\n"; f.close();
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6650.0f); CUR = &a;
        X.loadHostState(); X.syncSettings();
        // (DP 2.17.1, 2026-10-18 settings audit) settings live per MARKET in DealerProfile.settings-<MKT>.txt (the root-keyed file is
        // migrated once, left in place): the remembered key is the market, and the store re-reads at most once a second (SettingsSpec.h
        // ss::Store::load), so the test waits past that guard before expecting the later row
        check(X.savedKey == "ES", "no row for this chart: remembered by market (2.7.3 re-read the file on every draw)");
        check(X.cfg.font == 10 && X.cfg.labels, "and the defaults stay (10 pt)");
        std::ofstream g(fl("DealerProfile.settings.txt").c_str(), std::ios::app); g << "EPZ26|0|60|12|0|1|1|1|0|cob_bull\n"; g.close();
        X.savedStamp = -3;                                     // the file changed (mtime granularity is 1 s in the test)
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));   // past the store's once-a-second re-read guard (and the 1 s mtime grain)
        X.syncSettings();
        check(X.cfg.width == 60 && X.cfg.font == 12, "a row saved later is still applied");
        X.done();
    }
    {   // ---- 5. the status file is not rewritten on every repaint
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6650.0f); CUR = &a;
        X.loadHostState(); X.root = "EPZ26"; X.mkt = "ES";
        X.writeStatus("drawn"); std::remove(fl("DealerProfile.status.txt").c_str());
        X.writeStatus("drawn");
        check(!std::ifstream(fl("DealerProfile.status.txt").c_str()).is_open(), "same status again: no write");
        X.writeStatus("no data");
        check(std::ifstream(fl("DealerProfile.status.txt").c_str()).is_open(), "changed status: written");
        X.done();
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
