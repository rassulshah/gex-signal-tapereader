// test_hotreload_e2e_impl.cpp (2026-10-18) -- a stand-in implementation build for the Linux end-to-end test (built twice, as .so)
// It plays the SDK's part: cpp_main switches on the reason at offset 0 and keeps per-chart state in the slot at 0xA0.
#include "HotImpl.h"
#include <new>
#ifndef E2E_VERSION
#define E2E_VERSION "1.0.0"
#endif
#ifndef E2E_PARAMS
#define E2E_PARAMS "v1|output:Hot Reload Demo"
#endif
HR_IMPL_EXPORTS("lsHotReloadDemo", E2E_VERSION, E2E_PARAMS, 1, 7, 0)
struct St { long long build; int calcs; };
static int g_live = 0;                                    // charts this build holds state for
extern "C" HR_DLLEXPORT int e2e_live() { return g_live; }
extern "C" HR_DLLEXPORT int cpp_main(void* api)
{
    using namespace hotreload;
    int r = readReason(api);
    St* s = static_cast<St*>(readUser(api));
    switch (r) {
    case R_INIT: { int v = SDK_CHECKVERSION; std::memcpy(static_cast<char*>(api) + OFF_CHECKVERSION, &v, 4); return 0; }
    case R_SETUP: return 0;
    case R_CALC: case R_UPDT:
        if (!s) { s = new (std::nothrow) St{HR_BUILD, 0}; if (!s) return -1; writeUser(api, s); ++g_live; }
        s->calcs++;
        { long long b = s->build; std::memcpy(static_cast<char*>(api) + 0xC0, &b, 8); }   // which build ran (read by the test)
        return 0;
    case R_DESTR: if (s) { delete s; writeUser(api, nullptr); --g_live; } return 0;
    default: return 0;
    }
}
