#ifndef LS_CONTEXTREAD_H
#define LS_CONTEXTREAD_H
// ContextRead.h 0.2.0 (2026-10-26; 0.2.0 vocabulary v2: DATA / METRIC facts vs READ opinions, the class basis of each aware read) -- how EVERY indicator reads the shared context and its teammates the same way (L3), SDK-free.
// The 5-step process (TEAM-DESIGN.md sec. 6): raw read -> CTX(t) -> allowed teammates -> AWARE read (reasons + conf) -> record
// isolated + aware. Fallback: CTX late / missing or a teammate stale -> aware = isolated, flagged. Links ship OFF: a link is on only
// when LRA-Context-<MKT>.csv says "LINK,<id>,1" - switched on by Rassul after the nightly link test (item 2ac) proves it.
// Build aware reads MAINLY from teammates' DATA + METRIC (field / fieldD); a READ (opinion()) is a labelled opinion with its
// confidence and source, never a fact. Each aware read records its BASIS: the classes it used (D, M, R).
#include <map>
#include <set>
#include <string>
#include "MarketState.h"

namespace cr {

static const char* const VERSION = "0.2.0";

// ---- link switches (LRA-Context-<MKT>.csv text: "VERSION,1" / "MARKET,ES" / "LINK,L2,1" / "PARAM,TIMEOUT_MS,3000")
struct Links { std::map<std::string, bool> on; long long timeoutMs = 3000; std::string note = "defaults (all links OFF)"; };
inline Links parseLinks(const std::string& text, const std::string& market) {
    Links L; if (text.empty()) return L;
    std::string mkt; size_t a = 0; int good = 0;
    while (a <= text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(a, e - a); a = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> f = bb::split(line, ',', 4);
        if (f.size() >= 2 && f[0] == "MARKET") mkt = f[1];
        else if (f.size() >= 3 && f[0] == "LINK") { L.on[f[1]] = (f[2] == "1"); ++good; }
        else if (f.size() >= 3 && f[0] == "PARAM" && f[1] == "TIMEOUT_MS") { long long v = 0; if (bb::num(f[2], v) && v >= 250 && v <= 10000) L.timeoutMs = v; }
        if (e >= text.size()) break;
    }
    if (!mkt.empty() && mkt != market) { Links D; D.note = "file is for " + mkt + ", not " + market + " - defaults"; return D; }
    L.note = std::to_string(good) + " link lines";
    return L;
}
inline bool linkOn(const Links& L, const std::string& id) { auto it = L.on.find(id); return it != L.on.end() && it->second; }

// ---- reading CTX(t)
enum class CtxState { OK, LATE, MISSING };
struct Ctx { CtxState st = CtxState::MISSING; mst::Facts f; };
// waitedMs = how long this indicator has waited for CTX(t) since bar t closed (its own clock)
inline Ctx readCtx(const std::string& sym, long spb, const std::string& chart, long long nowMs, long long t, long long waitedMs, long long timeoutMs) {
    Ctx C; C.f = mst::read("CTX", sym, spb, chart, nowMs);
    if (C.f.ok && C.f.bar == t) { C.st = CtxState::OK; return C; }
    C.f = mst::Facts();                                   // a CTX for another bar is never used for t (no look-ahead, no stale context)
    C.st = waitedMs >= timeoutMs ? CtxState::MISSING : CtxState::LATE;
    return C;
}
// a CTX FACT (DATA or METRIC; "D.<k>" / "M.<k>"); false when CTX is not OK, the field is "?" - or it is a READ (opinions are
// never returned as facts: use opinion())
inline bool field(const Ctx& C, const std::string& k, std::string& v) {
    if (C.st != CtxState::OK) return false;
    for (const char* p : { "D.", "M." }) { auto it = C.f.kv.find(std::string(p) + k); if (it != C.f.kv.end()) { v = it->second; return v != "?"; } }
    return false;
}
inline char factClass(const Ctx& C, const std::string& k) {   // 'D' / 'M' / 'R' / 0 (absent) - for the basis of a read
    for (const char* p : { "D.", "M.", "R." }) if (C.f.kv.count(std::string(p) + k)) return p[0];
    return 0;
}
// a READ from CTX: a labelled opinion (value, the source's confidence or "?", the source)
struct Opinion { bool ok = false; std::string value, conf = "?", src = "?"; };
inline Opinion opinion(const Ctx& C, const std::string& k) {
    Opinion o; if (C.st != CtxState::OK) return o;
    auto it = C.f.kv.find("R." + k); if (it == C.f.kv.end() || it->second == "?") return o;
    const std::string& v = it->second; const size_t a = v.find('~'), b = a == std::string::npos ? a : v.find('~', a + 1);
    o.value = v.substr(0, a); if (a != std::string::npos) o.conf = v.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1);
    if (b != std::string::npos) o.src = v.substr(b + 1);
    o.ok = true; return o;
}
inline bool fieldD(const Ctx& C, const std::string& k, double& x) {
    std::string v; if (!field(C, k, v)) return false; char* e = nullptr; x = std::strtod(v.c_str(), &e); return e && *e == 0 && x == x;
}
// a teammate's facts for t (decided for <= t, fresh); nullptr-equivalent: ok = false
inline mst::Facts teammate(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long nowMs, long long t) {
    mst::Facts F = mst::read(src, sym, spb, chart, nowMs); if (F.ok && F.bar > t) F = mst::Facts(); return F;
}

// ---- the AWARE read
struct Read { int side = 0; int conf = 0; std::string word, reasons, basis; };   // basis = classes used: "D", "M", "R" (e.g. "DM")
struct Aware { Read isolated, aware; std::string ctx = "ok"; std::string links; };
// aware read = isolated unless the context is OK and at least one link changed it; the caller fills aware via its own rules
inline Aware finish(const Read& isolated, const Read* adjusted, CtxState st, const std::string& linksUsed) {
    Aware A; A.isolated = isolated; A.aware = isolated;
    A.ctx = st == CtxState::OK ? "ok" : st == CtxState::LATE ? "late" : "missing";
    if (st == CtxState::OK && adjusted) { A.aware = *adjusted; A.links = linksUsed; }
    else A.aware.basis.clear();                            // the isolated read uses no teammate
    return A;
}
inline void postAware(const std::string& src, const std::string& sym, long spb, const std::string& chart, long long nowMs, long long t, const Aware& A) {
    std::map<std::string, std::string> kv;
    kv["side"] = std::to_string(A.aware.side); kv["conf"] = std::to_string(A.aware.conf); kv["word"] = A.aware.word;
    kv["why"] = A.aware.reasons; kv["ctx"] = A.ctx; kv["links"] = A.links.empty() ? "-" : A.links;
    kv["iso_side"] = std::to_string(A.isolated.side); kv["iso_conf"] = std::to_string(A.isolated.conf);
    kv["class"] = "READ"; kv["basis"] = A.aware.basis.empty() ? "-" : A.aware.basis;   // an aware read is an opinion; what it was built from
    mst::publish("AW" + src, sym, spb, chart, nowMs, t, kv);
}
// the record line both answers go into (for the nightly link test; v2 adds the basis = classes the aware read used):
// AWR|2|<src>|<bar>|<ctx>|<links>|<basis>|<iso side>|<iso conf>|<aw side>|<aw conf>|<why>
inline std::string recordLine(const std::string& src, long long t, const Aware& A) {
    return "AWR|2|" + src + "|" + std::to_string(t) + "|" + A.ctx + "|" + (A.links.empty() ? "-" : mst::clean(A.links, 40)) + "|" +
           (A.aware.basis.empty() ? std::string("-") : mst::clean(A.aware.basis, 8)) + "|" +
           std::to_string(A.isolated.side) + "|" + std::to_string(A.isolated.conf) + "|" + std::to_string(A.aware.side) + "|" +
           std::to_string(A.aware.conf) + "|" + mst::clean(A.aware.reasons, 120);
}

}  // namespace cr
#endif
