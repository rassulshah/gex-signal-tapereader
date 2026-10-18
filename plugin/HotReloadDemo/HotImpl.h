#ifndef LS_HOT_IMPL_H
#define LS_HOT_IMPL_H
/********************************************************************************
 *  HotImpl.h  --  what an indicator adds to become a hot-reload implementation build   1.0.0 (2026-10-18)
 *
 *  Include it after irtsdk.h in the indicator's .cpp and put ONE line at file scope:
 *      HR_IMPL_EXPORTS("lsHotReloadDemo", DEMO_VERSION, DEMO_PARAMS, DEMO_ARRAYS, DEMO_FLAGS, DEMO_FLAGS_EXT)
 *  - the name is the shim DLL's name (the indicator name IRT shows)
 *  - PARAMS is one string listing the settings in order (type:name|...), the SAME list setup() declares; the shim compares its hash
 *    with the build IRT started with and refuses a build whose settings differ ("restart needed")
 *  - ARRAYS / FLAGS / FLAGS_EXT are the values CreateExtension passes to setArrayCount / setFlags / setExtendedFlags
 *  HR_BUILD comes from the build script (/DHR_BUILD=<n>, = the manifest BUILD); 0 = a plain build (the shim refuses it).
 *
 *  Rules for code that runs as a hot-reload build (docs/build/HOT-RELOAD.md, "Conversion rules"):
 *   1. setup() and init() run ONLY in the build IRT started with. Do not keep anything in members that setup() fills (e.g. parameter
 *      numbers from getParameterCount()): use constant parameter numbers.
 *   2. destroy() must release everything the chart owns (timers, controls, DEPTH objects, open files) - the shim calls DONE + DESTR
 *      on the old build when a chart moves to a new build.
 *   3. All per-chart state in the user-data slot (HostSlot.h) - already the rule; the shim keeps each build's pointer separate.
 *   4. Decisions are rebuilt from bars + the record files on the first calc of the new build (no repaint).
 ********************************************************************************/
#include "HotReloadLogic.h"
#include <cstring>

#ifndef HR_BUILD
#define HR_BUILD 0
#endif

namespace hotreload {
inline const HrHost*& implHost() { static const HrHost* h = nullptr; return h; }
// the shim's one-line status ("" = nothing to say); safe to call from draw()
inline std::string shimStatus()
{
    const HrHost* h = implHost();
    if (!h || h->size < sizeof(HrHost) || h->abi != ABI || !h->status) return std::string();
    char buf[256]; buf[0] = '\0';
    int n = h->status(buf, (int)sizeof buf);
    if (n < 0) n = 0;
    if (n > (int)sizeof buf - 1) n = (int)sizeof buf - 1;
    return std::string(buf, (size_t)n);
}
inline long long implBuild() { return (long long)HR_BUILD; }
inline void fillInfo(HrInfo* out, const char* name, const char* version, const char* params, int arrays, unsigned long flags,
                     unsigned long flagsExt)
{
    out->abi = ABI;
    out->build = (int64_t)HR_BUILD;
    std::memset(out->name, 0, sizeof out->name);
    std::memset(out->version, 0, sizeof out->version);
    std::strncpy(out->name, name, sizeof out->name - 1);
    std::strncpy(out->version, version, sizeof out->version - 1);
    out->paramSig = fnv1a(params);
    out->arrayCount = (int32_t)arrays;
    out->flags = (uint32_t)flags;
    out->flagsExt = (uint32_t)flagsExt;
}
} // namespace hotreload

#if defined(_WIN32)
#define HR_DLLEXPORT __declspec(dllexport)
#else
#define HR_DLLEXPORT __attribute__((visibility("default")))
#endif

#define HR_IMPL_EXPORTS(NAME, VERSION, PARAMS, ARRAYS, FLAGS, FLAGS_EXT)                                          \
    extern "C" HR_DLLEXPORT int hr_info(hotreload::HrInfo* out)                                                   \
    {                                                                                                             \
        if (!out || out->size != sizeof(hotreload::HrInfo)) return 0;                                             \
        hotreload::fillInfo(out, NAME, VERSION, PARAMS, (int)(ARRAYS), (unsigned long)(FLAGS), (unsigned long)(FLAGS_EXT)); \
        return 1;                                                                                                 \
    }                                                                                                             \
    extern "C" HR_DLLEXPORT void hr_attach(const hotreload::HrHost* h) { hotreload::implHost() = h; }

#endif
