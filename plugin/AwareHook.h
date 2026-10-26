#ifndef LS_AWAREHOOK_H
#define LS_AWAREHOOK_H
// AwareHook.h 0.1.0 (2026-10-26) -- the smart-read hook (layer L3) every teammate calls once per pass, SDK-free (unit-tested on Linux).
// Rassul 2026-10-26 12:53 CT: "build the team and the processes of communication and how they work together". Design: TEAM-DESIGN.md.
//
// For each CLOSED bar t the indicator has decided its own (isolated) read for:
//   1. wait until lsContext has posted CTX(t) (state OK) or this indicator has waited the timeout (LATE -> MISSING)
//   2. decide ONCE: aware = isolated while every link of this market is OFF (the default; LRA-Context-<MKT>.csv) - nothing it shows changes
//   3. post AW<src> to MarketState (lsContext builds TEAM(t) from them) and re-post it unchanged every 15 s
//   4. append the AWR record line (isolated + aware) to lsFlexLevels\Context-AWR-<MKT>-<spb>s-<yyyymmdd>.rec for the nightly link test
// A read for t is never changed after it is decided, and a CTX for another bar is never used (no look-ahead).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include "ContextRead.h"

namespace awh {

static const char* const VERSION = "0.1.0";

inline void civilFromDays(long long z, int& y, int& m, int& d) {
    z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153; d = (int)(doy - (153 * mp + 2) / 5 + 1); m = (int)(mp < 10 ? mp + 3 : mp - 9); y = (int)(yoe + era * 400 + (m <= 2));
}
// the yyyymmdd of a MarketState bar id (CT wall-clock seconds of the bar's end stamp)
inline std::string dayOf(long long t) { int y, m, d; civilFromDays(t >= 0 ? t / 86400 : (t - 86399) / 86400, y, m, d); char b[16]; std::snprintf(b, sizeof(b), "%04d%02d%02d", y, m, d); return b; }
inline std::string flexDir() {
#if defined(_WIN32) && !defined(HB_FAKE)
    const char* up = std::getenv("USERPROFILE"); return (up && up[0]) ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string();
#else
    const char* d = std::getenv("LRA_FLEX_DIR"); return d ? std::string(d) : std::string();
#endif
}
inline std::string sep() {
#if defined(_WIN32) && !defined(HB_FAKE)
    return "\\";
#else
    return "/";
#endif
}

// ---- the isolated reads of the first four teammates (from what each already decided for t)
// Reversal Stage: side only from Collision (2) on, and not on a failed / expired attempt; conf 0 = not calibrated yet (the trust table learns it)
inline cr::Read stageRead(int stage, bool lowPivot, int status /* 2 failed, 4 expired */) {
    cr::Read r; r.word = "stage " + std::to_string(stage);
    if (stage >= 2 && status != 2 && status != 4) { r.side = lowPivot ? 1 : -1; r.reasons = std::string(lowPivot ? "low" : "high") + " pivot at stage " + std::to_string(stage); }
    else r.reasons = stage <= 0 ? "no attempt" : (status == 2 ? "attempt failed" : status == 4 ? "attempt expired" : "before collision");
    return r;
}
// Delta Profile: a FINAL A on this bar gives its side (a tentative A gives none)
inline cr::Read dpRead(const std::map<std::string, std::string>& kv) {
    cr::Read r; auto g = [&](const char* k) { auto it = kv.find(k); return it == kv.end() ? std::string("?") : it->second; };
    const std::string a = g("a"), s = g("a_side");
    r.word = a == "none" || a == "?" ? "no A" : a + " A";
    if (a == "final" && (s == "bull" || s == "bear")) { r.side = s == "bull" ? 1 : -1; r.reasons = s + " A " + g("a_x") + "x at " + g("a_px"); }
    else r.reasons = a == "tentative" ? "A not final" : "no A on this bar";
    return r;
}
// facts-only teammates (aggression, HOD / LOD): no side of their own yet; their facts reach the team through CTX
inline cr::Read factsRead(const std::string& word) { cr::Read r; r.word = word; r.reasons = "facts only"; return r; }

struct Hook {
    std::string src, dir;                  // dir empty = flexDir()
    long long t = -1, firstMs = -1, postMs = -1, timeoutMs = 3000;
    bool decided = false; cr::Aware A;
    long writes = 0, writeFails = 0;
    // call on every pass with the last closed bar t this indicator has decided its isolated read for.
    // returns true on the pass that decided (and recorded) the read for t.
    bool tick(const std::string& sym, long spb, const std::string& chart, const std::string& market, long long nowMs, long long tNow, const cr::Read& iso) {
        if (sym.empty() || tNow <= 0) return false;
        if (tNow != t) { t = tNow; firstMs = nowMs; decided = false; postMs = -1; }
        bool did = false;
        if (!decided) {
            const cr::Ctx C = cr::readCtx(sym, spb, chart, nowMs, t, nowMs - firstMs, timeoutMs);
            if (C.st == cr::CtxState::LATE) return false;                               // wait (lsContext builds CTX(t) after our facts for t)
            A = cr::finish(iso, nullptr, C.st, "");                                      // links OFF: aware = isolated (no rule changes it yet)
            decided = true; did = true;
            const std::string d = dir.empty() ? flexDir() : dir;
            if (!d.empty()) {
                std::ofstream f((d + sep() + "Context-AWR-" + market + "-" + std::to_string(spb) + "s-" + dayOf(t) + ".rec").c_str(), std::ios::app | std::ios::binary);
                if (f) { f << cr::recordLine(src, t, A) << "\n"; if (f.good()) ++writes; else ++writeFails; } else ++writeFails;
            }
        }
        if (postMs < 0 || nowMs < postMs || nowMs - postMs >= 15000) { cr::postAware(src, sym, spb, chart, nowMs, t, A); postMs = nowMs; }
        return did;
    }
    void clear(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) mst::clear("AW" + src, sym, spb, chart); t = -1; decided = false; }
};

}  // namespace awh
#endif
