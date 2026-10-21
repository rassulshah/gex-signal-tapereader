// SettingsGuard.h 1.1.0 (2026-10-20 night; 1.1.0 2026-10-21) - the settings window's calls run behind a Windows exception guard, and the
// chart's market is read without ever faulting.
// (1.1.0, Rassul 2026-10-21 09:55 "you have had constant problems with allowing the user to change settings . fix this issue") msglog
// 09:51: "C0000005 ... lsSessionVWAP Ver 1.6.1 in chart "???" calling parmsLoad()" - the window opened with VWAP / bands unticked and
// a black colour while the chart drew them. The 1.0.0 guard had been put into 3 of 30 plugins only, and a guarded copy with no chart
// still opened UNFILLED. Now ONE macro, SS_SAFE_DIALOG(Class), in every plugin with a settings window (tests_settings/
// test_dialog_guard.py fails the build otherwise):
//   - parmsLoad / parmsApply / parmsUpdt run the plugin's parmsLoadBody / parmsApplyBody / parmsUpdtBody behind ss::sehRun;
//   - getRootSymbol / getSymbol are shadowed by guarded versions (a fault or no chart -> NULL, never a crash);
//   - a window copy with NO chart, while it LOADS, is given the market of this plugin's last window that knew its chart (this IRT
//     run, else the file lsFlexLevels\Settings-lastmarket-<Plugin>.txt, else ES): the window opens FILLED (snapshot taken), so OK
//     on the real chart saves only the controls he changed (SettingsHost Snap). Apply / change with no chart never guesses (no save);
//   - every window call is logged in lsFlexLevels\Settings-dialog.log (time, plugin, call, chart known / guessed / fault) so the next
//     settings complaint is answered from evidence, not guesses (BUILD-LESSONS 119).
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
#include <cstdio>
#include <cstdlib>
#include <ctime>
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

struct DialogCtx { const void* loading = nullptr; std::string plugin; std::string guessed; bool faulted = false; };
inline DialogCtx& dialogCtx() { static DialogCtx c; return c; }                         // one per DLL (each plugin compiles its own copy)
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
    if (m < 0) { const DialogCtx& g = dialogCtx(); if (g.loading == (const void*)x && marketIndex(g.guessed) >= 0) return g.guessed; }   // (1.1.0) a chart-less copy while it loads
    return m < 0 ? std::string() : std::string(MARKETS[m]);
}

// run a member call of the plugin behind the guard: true = a fault was caught
template <class T> struct GuardCall { T* me; void (T::*fn)(unsigned); unsigned i; static void run(void* p) { GuardCall* a = static_cast<GuardCall*>(p); (a->me->*(a->fn))(a->i); } };
template <class T> inline bool guardedCall(T* me, void (T::*fn)(unsigned), unsigned i) { GuardCall<T> a; a.me = me; a.fn = fn; a.i = i; return sehRun(&GuardCall<T>::run, &a) != 0; }

// ------------------------------------------------------------------------------------------------ (1.1.0) one macro for every plugin
inline std::string flexDirGuard() {
    const char* h = getenv("USERPROFILE"); if (!h || !h[0]) return std::string();
#if defined(_WIN32)
    return std::string(h) + "\\InvestorRT\\rtx\\lsFlexLevels\\";
#else
    return std::string(h) + "/InvestorRT/rtx/lsFlexLevels/";
#endif
}
inline std::string& lastDialogMarket() { static std::string m; return m; }
inline std::string lastMarketFile(const std::string& plugin) { const std::string d = flexDirGuard(); return d.empty() ? d : d + "Settings-lastmarket-" + plugin + ".txt"; }
inline void noteDialogMarket(const std::string& plugin, const std::string& mkt) {
    if (marketIndex(mkt) < 0 || mkt == lastDialogMarket()) return;
    lastDialogMarket() = mkt;
    const std::string p = lastMarketFile(plugin); if (p.empty()) return;
    if (FILE* f = fopen(p.c_str(), "wb")) { fputs(mkt.c_str(), f); fclose(f); }
}
// the market a chart-less window copy opens on: this run's last window with a chart, else the file, else ES
inline std::string guessDialogMarket(const std::string& plugin) {
    if (marketIndex(lastDialogMarket()) >= 0) return lastDialogMarket();
    const std::string p = lastMarketFile(plugin);
    if (!p.empty()) if (FILE* f = fopen(p.c_str(), "rb")) { char b[8] = {0}; size_t n = fread(b, 1, 7, f); fclose(f); b[n] = 0; std::string m(b); if (marketIndex(m) >= 0) return m; }
    return "ES";
}
inline const char* rootOfMarket(const std::string& m) { return m == "EU" ? "6E" : m.c_str(); }   // marketOfRoot("6E") = EU
inline void dialogLog(const std::string& plugin, const char* call, const std::string& how) {
    const std::string d = flexDirGuard(); if (d.empty()) return;
    const std::string p = d + "Settings-dialog.log";
    long sz = 0; if (FILE* r = fopen(p.c_str(), "rb")) { fseek(r, 0, SEEK_END); sz = ftell(r); fclose(r); }
    if (sz > 400000) { const std::string o = p + ".old"; remove(o.c_str()); rename(p.c_str(), o.c_str()); }
    if (FILE* f = fopen(p.c_str(), "ab")) {
        time_t t = time(nullptr); char ts[32] = {0}; struct tm lt; 
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &lt);
        fprintf(f, "%s,%s,%s,%s\n", ts, plugin.c_str(), call, how.c_str()); fclose(f);
    }
}
template <class T> struct BodyCall { T* me; int (T::*f0)(); int (T::*f1)(unsigned int); unsigned i; int rc;
    static void run(void* p) { BodyCall* a = static_cast<BodyCall*>(p); a->rc = a->f1 ? (a->me->*(a->f1))(a->i) : (a->me->*(a->f0))(); } };
// one window call: the chart market read safely (guess only while loading a chart-less copy), the body behind the guard, the log line
template <class T> inline int dialogCall(T* me, const char* plugin, const char* call, int (T::*f0)(), int (T::*f1)(unsigned int), unsigned i) {
    DialogCtx& c = dialogCtx();
    const bool load = std::string(call) == "load";
    bool symFault = false; const std::string real = safeMarket(me, &symFault);
    const bool dlg = inDialog(me);
    if (dlg && !real.empty()) noteDialogMarket(plugin, real);
    c.plugin = plugin; c.faulted = false; c.guessed.clear();
    if (load && dlg && real.empty()) c.guessed = guessDialogMarket(plugin);
    c.loading = load ? (const void*)me : nullptr;
    BodyCall<T> a; a.me = me; a.f0 = f0; a.f1 = f1; a.i = i; a.rc = RTX_OK;
    const bool fault = sehRun(&BodyCall<T>::run, &a) != 0;
    c.loading = nullptr;
    if (dlg) dialogLog(plugin, call, std::string(real.empty() ? (c.guessed.empty() ? "no chart (not saved)" : "no chart, opened on " + c.guessed) : "chart " + real)
                                     + (symFault || c.faulted ? ", symbol fault caught" : "") + (fault ? ", FAULT in body caught" : "") + (f1 ? ", item " + std::to_string(i) : std::string()));
    return fault ? RTX_OK : a.rc;
}
// the shadowed symbol calls: a fault -> NULL; a chart-less window copy while it loads -> the guessed market's root
template <class X> inline auto baseRoot(X* x, char* r, const char* t, int) -> decltype((const char*)x->X::getRootSymbol(r, t)) { return x->X::getRootSymbol(r, t); }
template <class X> inline const char* baseRoot(X* x, char* r, const char*, long) { return x->X::getRootSymbol(r); }   // older mocks: one argument
struct RootCall { cppExtension* x; char* r; const char* t; const char* got; static void run(void* p) { RootCall* a = static_cast<RootCall*>(p); a->got = baseRoot<cppExtension>(a->x, a->r, a->t, 0); } };
struct SymOnly { cppExtension* x; const char* got; static void run(void* p) { SymOnly* a = static_cast<SymOnly*>(p); a->got = a->x->cppExtension::getSymbol(); } };
inline char* guessedRoot(cppExtension* x, char* buf) {
    DialogCtx& c = dialogCtx(); if (c.loading != (const void*)x || c.guessed.empty()) return nullptr;
    static char sb[8]; char* o = buf ? buf : sb; const char* r = rootOfMarket(c.guessed); size_t n = 0; for (; r[n] && n < 7; ++n) o[n] = r[n]; o[n] = 0; return o;
}
inline char* safeRootSymbol(cppExtension* x, char* r, const char* t) {
    RootCall a; a.x = x; a.r = r; a.t = t; a.got = nullptr;
    if (sehRun(&RootCall::run, &a) != 0) { dialogCtx().faulted = true; a.got = nullptr; }
    if (t) return const_cast<char*>(a.got);                                  // a lookup of a named ticker: not about this chart
    if (a.got && a.got[0] && marketOfRoot(a.got) >= 0) return const_cast<char*>(a.got);
    char* g = guessedRoot(x, r); return g ? g : const_cast<char*>(a.got);
}
inline char* safeSymbol(cppExtension* x) {
    SymOnly a; a.x = x; a.got = nullptr;
    if (sehRun(&SymOnly::run, &a) != 0) { dialogCtx().faulted = true; a.got = nullptr; }
    if (a.got && a.got[0] && marketOfRoot(a.got) >= 0) return const_cast<char*>(a.got);
    char* g = guessedRoot(x, nullptr); return g ? g : const_cast<char*>(a.got);
}

} // namespace ss

// Put in the PUBLIC part of every plugin class with a settings window; the plugin's own window code is renamed parmsLoadBody (void),
// parmsApplyBody (void), parmsUpdtBody (unsigned int) - all three must exist (an empty body returns RTX_OK).
#define SS_SAFE_DIALOG(Cls, NAME) \
    virtual int parmsLoad(void) { return ss::dialogCall<Cls>(this, NAME, "load", &Cls::parmsLoadBody, nullptr, 0u); } \
    virtual int parmsApply(void) { return ss::dialogCall<Cls>(this, NAME, "apply", &Cls::parmsApplyBody, nullptr, 0u); } \
    virtual int parmsUpdt(unsigned int i) { return ss::dialogCall<Cls>(this, NAME, "change", nullptr, &Cls::parmsUpdtBody, i); } \
    char* getRootSymbol(char* r = NULL, const char* t = NULL) { return ss::safeRootSymbol(this, r, t); } \
    char* getSymbol(void) { return ss::safeSymbol(this); }

namespace ss {

} // namespace ss
