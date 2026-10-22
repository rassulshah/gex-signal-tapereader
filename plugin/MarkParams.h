// MarkParams.h 1.0.0 (2026-10-22 evening, Rassul approved the build 17:39 CT): the SHARED, SDK-free reader of the nightly's per-market
// SHOW / LOG params - lsFlexLevels\LRA-<Name>-<MKT>.csv - so every drawn MBO / context mark can be switched between SHOW, SHOW? and LOG
// (record only) per market by the nightly, without a rebuild ("apply today's validation verdicts to what is drawn", VALIDATE-CONTEXT-MARKS
// and VALIDATE-DEFENDER-SIZE 2026-10-22). Also reads LRA-FeedCheck-<MKT>.csv for NewsWindow.h.
//   File: "VERSION,<n>" first (comments '#' and blank lines skipped), "MARKET,<MKT>" (must match the chart's market), then
//   "PARAM,<name>,<value>" lines; any other row (e.g. "EVENT,...") is kept in rows for the caller. Garbled = no values (the caller's
//   defaults) + the reason in why. Bounded (64 KiB, 512 lines), torn-safe (stamp before / after), re-read only when the stamp changes,
//   checked at most every 3 s; a missing file is cached as missing.
// Usage: mp::mode("Trails", "ES", "SHOW", mp::LOG) -> mp::LOG / mp::SHOWQ / mp::SHOW (values SHOW / SHOW? / LOG / 1 / 0).
#pragma once
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

namespace mp {
static const char* const VERSION = "1.0.0";
enum Mode { LOG = 0, SHOWQ = 1, SHOW = 2 };

struct Params {
    bool ok = false, missing = true;
    std::string why = "not read", path;
    std::map<std::string, std::string> kv;              // PARAM name -> value (upper-case name)
    std::vector<std::vector<std::string> > rows;        // other rows (first cell upper-case), e.g. EVENT lines
    long long stampSz = -1, stampMt = -1; long long checkedMs = -100000;
};

inline std::string upper(std::string s) { for (size_t i = 0; i < s.size(); ++i) if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 32); return s; }
inline std::string trim(const std::string& s) { size_t a = 0, b = s.size(); while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a; while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b; return s.substr(a, b - a); }
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; size_t a = 0; for (;;) { const size_t b = s.find(',', a); o.push_back(trim(s.substr(a, b == std::string::npos ? std::string::npos : b - a))); if (b == std::string::npos) break; a = b + 1; } return o; }

// the chart market as the files name it ("6E" charts are "EU" in every LRA file)
inline std::string normMarket(const std::string& m) { const std::string u = upper(trim(m)); return u == "6E" ? std::string("EU") : u; }

inline std::string flexDir() {
    const char* up = std::getenv("USERPROFILE"); if (!up || !*up) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";   // the plugins' convention (on the Linux mock hosts too)
}
inline std::string pathOf(const std::string& name, const std::string& mkt) {
    const std::string f = flexDir(); if (f.empty() || name.empty() || mkt.empty()) return std::string();
    return f + "\\LRA-" + name + "-" + mkt + ".csv";
}
inline bool statOf(const std::string& p, long long& sz, long long& mt) { struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) return false; sz = (long long)st.st_size; mt = (long long)st.st_mtime; return true; }
inline long long nowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

// pure parse (unit-tested): text -> Params for market mkt
inline Params parse(const std::string& text, const std::string& mkt) {
    Params P; P.missing = false;
    if (text.size() > 65536) { P.why = "file too big - defaults"; return P; }
    bool ver = false, mk = false; size_t a = 0; int lines = 0;
    std::map<std::string, std::string> kv; std::vector<std::vector<std::string> > rows;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        const std::string ln = trim(text.substr(a, b - a)); a = b + 1;
        if (++lines > 512) { P.why = "too many lines - defaults"; return P; }
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> c = split(ln); c[0] = upper(c[0]);
        if (!ver) {
            if (c[0] != "VERSION" || c.size() < 2 || c[1].empty()) { P.why = "garbled (no VERSION first) - defaults"; return P; }
            char* e = nullptr; const long v = std::strtol(c[1].c_str(), &e, 10); if (!e || *e || v < 1) { P.why = "garbled VERSION - defaults"; return P; }
            ver = true; continue;
        }
        if (c[0] == "MARKET") { if (c.size() < 2 || normMarket(c[1]) != normMarket(mkt)) { P.why = "wrong MARKET - defaults"; return P; } mk = true; continue; }
        if (!mk) { P.why = "garbled (no MARKET before values) - defaults"; return P; }
        if (c[0] == "PARAM") { if (c.size() < 3 || c[1].empty()) { P.why = "garbled PARAM line - defaults"; return P; } kv[upper(c[1])] = c[2]; continue; }
        rows.push_back(c);
    }
    if (!ver || !mk) { P.why = !ver ? "empty - defaults" : "no MARKET - defaults"; return P; }
    P.ok = true; P.why = "ok"; P.kv.swap(kv); P.rows.swap(rows); return P;
}

// cached read (thread-safe; one cache per DLL)
inline std::mutex& mtx() { static std::mutex m; return m; }
inline std::map<std::string, Params>& cache() { static std::map<std::string, Params> c; return c; }
inline Params get(const std::string& name, const std::string& mktIn) {
    const std::string mkt = normMarket(mktIn);
    const std::string p = pathOf(name, mkt);
    std::lock_guard<std::mutex> g(mtx());
    Params& C = cache()[name + "|" + mkt];
    const long long now = nowMs();
    if (now - C.checkedMs < 3000 && C.checkedMs > 0 && C.path == p) return C;
    C.checkedMs = now; C.path = p;
    long long sz = -1, mt = -1;
    if (!statOf(p, sz, mt)) { Params M; M.missing = true; M.why = "no file - defaults"; M.path = p; M.checkedMs = now; C = M; return C; }
    if (sz == C.stampSz && mt == C.stampMt) return C;
    if (sz > 65536) { Params M; M.missing = false; M.why = "file too big - defaults"; M.path = p; M.checkedMs = now; M.stampSz = sz; M.stampMt = mt; C = M; return C; }
    std::string text; { FILE* f = std::fopen(p.c_str(), "rb"); if (!f) return C; char buf[4096]; size_t n; while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && text.size() < 70000) text.append(buf, n); std::fclose(f); }
    long long sz2 = -1, mt2 = -1;
    if (!statOf(p, sz2, mt2) || sz2 != sz || mt2 != mt || (long long)text.size() != sz) return C;   // torn: keep the last good copy, read again later
    Params P = parse(text, mkt); P.path = p; P.checkedMs = now; P.stampSz = sz; P.stampMt = mt; C = P; return C;
}

inline int modeOf(const std::string& vIn, int dflt) {
    const std::string v = upper(trim(vIn));
    if (v == "SHOW" || v == "1" || v == "2") return SHOW;
    if (v == "SHOW?" || v == "Q") return SHOWQ;
    if (v == "LOG" || v == "0") return LOG;
    return dflt;
}
inline int mode(const Params& P, const std::string& key, int dflt) { std::map<std::string, std::string>::const_iterator it = P.kv.find(upper(key)); return P.ok && it != P.kv.end() ? modeOf(it->second, dflt) : dflt; }
inline int mode(const std::string& name, const std::string& mkt, const std::string& key, int dflt) { return mode(get(name, mkt), key, dflt); }
inline double num(const Params& P, const std::string& key, double dflt, double lo, double hi) {
    std::map<std::string, std::string>::const_iterator it = P.kv.find(upper(key)); if (!P.ok || it == P.kv.end() || it->second.empty()) return dflt;
    char* e = nullptr; const double v = std::strtod(it->second.c_str(), &e); if (!e || *e || !std::isfinite(v) || v < lo || v > hi) return dflt; return v;
}
inline const char* modeName(int m) { return m == SHOW ? "SHOW" : m == SHOWQ ? "SHOW?" : "LOG"; }
} // namespace mp
