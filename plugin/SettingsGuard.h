// SettingsGuard.h 1.0.0 (2026-10-20 night) - the settings window's calls run behind a Windows exception guard, and the chart's market
// is read without ever faulting.
// WHY (IRT admin\msglog.txt): "C0000005 Exception at 7FFD0EE89BCF in RTX extension "lsBigBidAsk" Ver 1.19.0 in chart "???" calling
// parmsLoad()" and then "... in chart "NQ LTF" calling parmsApply()" (2026-10-20 22:24, Rassul copying Big Bid / Ask to NQ: the window
// showed Biggest in 0 / Skip first 9 / Skip last ??? and OK saved nothing); the same address in lsDefenseLines 1.1.0 (parmsLoad + Apply)
// and lsBigBidAsk 1.16.1 (parmsLoad). The address is outside the plugin (a Windows / runtime DLL) and every time the window copy had
// NO chart ("???" = an indicator being added or pasted). IRT aborted the call: the window was never filled from the file, and the
// Apply that followed died the same way - so nothing was saved.
// WHAT: ss::sehRun runs a call and catches a hardware fault (access violation) instead of letting it abort the window; ss::safeMarket
// reads the chart's root / symbol inside that guard (a copy with no chart -> "" = no market: the window keeps the defaults and is
// never saved, so the chart then uses ITS market's file). Include after SettingsHost.h. On non-MSVC builds (Linux tests, MinGW syntax
// check) the guard is a plain call.
#pragma once
#include "SettingsHost.h"
#include <string>
#if defined(_MSC_VER)
#include <excpt.h>
#endif

namespace ss {

typedef void (*GuardFn)(void*);
#if defined(_MSC_VER)
inline int sehRun(GuardFn fn, void* arg) { __try { fn(arg); return 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return 1; } }
#else
inline int sehRun(GuardFn fn, void* arg) { fn(arg); return 0; }
#endif

struct SymCall { cppExtension* x; char buf[64]; const char* got; };
// the text is copied INSIDE the guard (a bad pointer faults here, not later)
inline void copyIn(SymCall* c, const char* t) { char out[64]; int i = 0; if (t) for (; i < 63 && t[i]; ++i) out[i] = t[i]; out[i] = 0; for (int k = 0; k <= i; ++k) c->buf[k] = out[k]; c->got = t ? c->buf : nullptr; }
inline void rootThunk(void* p) { SymCall* c = static_cast<SymCall*>(p); char b[64]; for (int i = 0; i < 64; ++i) b[i] = 0; const char* t = c->x->getRootSymbol(b); copyIn(c, t); }
inline void symThunk(void* p) { SymCall* c = static_cast<SymCall*>(p); copyIn(c, c->x->getSymbol()); }

// the chart's market ("ES" .. "EU") or "" when the copy has no chart / the call faults; *faulted = a fault was caught
inline std::string safeMarket(cppExtension* x, bool* faulted = nullptr) {
    if (faulted) *faulted = false;
    SymCall c; c.x = x; c.got = nullptr; for (int i = 0; i < 64; ++i) c.buf[i] = 0;
    std::string rs;
    if (sehRun(rootThunk, &c) == 0) { if (c.got) rs = c.buf; } else if (faulted) *faulted = true;
    int m = marketOfRoot(rs);
    if (m < 0) {
        SymCall s; s.x = x; s.got = nullptr; for (int i = 0; i < 64; ++i) s.buf[i] = 0;
        std::string sy;
        if (sehRun(symThunk, &s) == 0) { if (s.got) sy = s.buf; } else if (faulted) *faulted = true;
        m = marketOfRoot(sy);
    }
    return m < 0 ? std::string() : std::string(MARKETS[m]);
}

// run a member call of the plugin behind the guard: true = a fault was caught
template <class T> struct GuardCall { T* me; void (T::*fn)(unsigned); unsigned i; static void run(void* p) { GuardCall* a = static_cast<GuardCall*>(p); (a->me->*(a->fn))(a->i); } };
template <class T> inline bool guardedCall(T* me, void (T::*fn)(unsigned), unsigned i) { GuardCall<T> a; a.me = me; a.fn = fn; a.i = i; return sehRun(&GuardCall<T>::run, &a) != 0; }

} // namespace ss
