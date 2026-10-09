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

static int passes = 0, fails = 0;
static void check(bool ok, const char* m) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }

// two Globex sessions of 3-min ES bars: Wed 17:00 .. Thu 16:00 (RTH high = base + 10) then Thu 17:00 .. Fri 10:00
static void build(Chart& C, float base)
{
    const unsigned long wed1700 = (unsigned long)svl::daysFromCivil(2026, 10, 7) * 86400UL + 17UL * 3600UL;
    for (unsigned long t = wed1700 + 180; t <= wed1700 + 23UL * 3600UL; t += 180) {           // to Thu 16:00
        int sec = (int)(t % 86400); float hi = base + 1, lo = base - 1;
        if (sec == 10 * 3600) hi = base + 10;                                                  // Thu 10:00 RTH high
        C.dt.push_back(t); C.o.push_back(base); C.h.push_back(hi); C.l.push_back(lo); C.c.push_back(base); C.v.push_back(10);
    }
    const unsigned long thu1700 = wed1700 + 86400UL;
    for (unsigned long t = thu1700 + 180; t <= thu1700 + 17UL * 3600UL; t += 180) {           // to Fri 10:00 (RTH started)
        C.dt.push_back(t); C.o.push_back(base); C.h.push_back(base + 1); C.l.push_back(base - 1); C.c.push_back(base); C.v.push_back(10);
    }
}
static double pdh(const std::vector<klv::Merged>& L)
{
    for (const auto& m : L) if (("/" + m.name + "/").find("/PDH/") != std::string::npos) return m.price;
    return -1;
}
static double levelsOf(DealerProfile& X)       // one draw's key-level step, exactly as draw() wraps it
{
    if (!X.loadHostState()) return -2;
    X.mkt = "ES";
    double r = pdh(X.nativeLevels("ES"));
    X.saveHostState();
    return r;
}

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    {   // two ES charts (different contracts / prices) on one DLL object, drawn back to back (well inside the 10-s throttle)
        DealerProfile X;
        Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6700.0f);
        Chart b; b.root = "ES"; b.sym = "EPH27"; build(b, 6760.0f);
        CUR = &a; double pa = levelsOf(X);
        CUR = &b; double pb = levelsOf(X);
        CUR = &a; double pa2 = levelsOf(X);
        check(pa == 6710.0, "chart A: its own prior-day high");
        check(pb == 6770.0, "chart B: its OWN prior-day high, not chart A's (shared-cache leak)");
        check(pa2 == 6710.0, "chart A again: still its own");
        CUR = &a; X.done(); CUR = &b; X.done();
        check(a.user == nullptr && b.user == nullptr, "done releases both charts' state");
    }
    {   // one chart: the cache still works (the second draw within 10 s reuses it; a new bar recomputes)
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; build(a, 6700.0f);
        CUR = &a; double p1 = levelsOf(X); double p2 = levelsOf(X);
        check(p1 == 6710.0 && p2 == 6710.0, "single chart: same levels on every draw");
#ifdef RA1009_PATCHED
        DealerProfile::State* s = static_cast<DealerProfile::State*>(a.user);
        check(s && s->klc.count("ES") && s->klc["ES"].n == (long)a.dt.size(), "single chart: the cache lives in this chart's host state");
#endif
        CUR = &a; X.destroy();
    }
    {   // a host that keeps no user-data slot: HostSlot's one shared state (the pre-audit behaviour) - no crash, no leak
        DealerProfile X; Chart a; a.root = "ES"; a.sym = "EPZ26"; a.keepsSlot = false; build(a, 6700.0f);
        CUR = &a; check(levelsOf(X) == 6710.0, "no-slot host: levels computed");
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
